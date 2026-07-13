#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
  constexpr int watchIntervalMs = 400;
}

QuillAudioProcessor::QuillAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {
  compilerResult = CompilerLocator::locate();

  buildRoot = juce::File::getSpecialLocation(juce::File::tempDirectory)
                .getChildFile("quill_dsp")
                .getChildFile(juce::Uuid().toString());

  if (compilerResult.found) {
    // navigate to shared using compile-time source path
    auto sharedDir = juce::File(__FILE__).getParentDirectory().getSiblingFile("shared");
    compileService = std::make_unique<CompileService>(compilerResult.path, sharedDir);
  }
}

QuillAudioProcessor::~QuillAudioProcessor() {
  processFn.store(nullptr, std::memory_order_relaxed);
  compileService.reset();
  retireQueue.clear();
  activeDsp.reset();

  // sweep instance's builds
  buildRoot.deleteRecursively();
}

void QuillAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
  currentSampleRate = sampleRate;
  currentBlockSize  = samplesPerBlock;

  if (activeDsp != nullptr) {
    activeDsp->prepare(sampleRate, samplesPerBlock);
  }
}

void QuillAudioProcessor::releaseResources() {
  processFn.store(nullptr, std::memory_order_release);
}

bool QuillAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
    && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }

  return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void QuillAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;

  auto fn = processFn.load(std::memory_order_acquire);
  if (fn == nullptr) {
    return;
  }

  // route through hot-swapped dsp function on every channel
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    fn(buffer.getReadPointer(ch), buffer.getWritePointer(ch), buffer.getNumSamples());
  }
}

void QuillAudioProcessor::setSourceFile(const juce::File& file) {
  sourceFile = file;
  lastPolledModTime = juce::Time();
  lastCompiledModTime = juce::Time();
  startTimer(watchIntervalMs);

  if (!sourceFile.existsAsFile()) {
    setDspState(DspState::Missing);
    return;
  }

  if (armed) {
    startCompile();
  } else {
    setDspState(DspState::Idle);
  }
}

juce::File QuillAudioProcessor::getSourceFile() const {
  return sourceFile;
}

QuillAudioProcessor::DspState QuillAudioProcessor::getDspState() const {
  return dspState;
}

bool QuillAudioProcessor::isArmed() const {
  return armed;
}

void QuillAudioProcessor::arm() {
  if (armed || !sourceFile.existsAsFile()) {
    return;
  }

  armed = true;
  armWhenFound = false;
  startCompile();
}

void QuillAudioProcessor::disarm() {
  if (!armed) {
    return;
  }

  armed = false;
  armWhenFound = false;
  stopDsp();

  if (sourceFile == juce::File()) {
    setDspState(DspState::Empty);
  } else {
    setDspState(sourceFile.existsAsFile() ? DspState::Idle : DspState::Missing);
  }
}

void QuillAudioProcessor::timerCallback() {
  if (sourceFile == juce::File()) {
    return;
  }

  if (!sourceFile.existsAsFile()) {
    // file vanished
    if (dspState == DspState::Idle) {
      setDspState(DspState::Missing);
    }
    return;
  }

  // file back
  if (dspState == DspState::Missing) {
    setDspState(DspState::Idle);

    if (armWhenFound) {
      arm();
      return;
    }
  }

  if (!armed) {
    return;
  }

  // wait until modtime is stable for full tick so half-written save never compiles
  auto modTime = sourceFile.getLastModificationTime();
  if (modTime != lastPolledModTime) {
    lastPolledModTime = modTime;
    return;
  }

  if (modTime != lastCompiledModTime && !compileInFlight) {
    startCompile();
  }
}

void QuillAudioProcessor::setDspState(DspState newState) {
  if (dspState != newState) {
    dspState = newState;
    sendChangeMessage();
  }
}

void QuillAudioProcessor::startCompile() {
  if (!sourceFile.existsAsFile()) {
    return;
  }

  if (compileService == nullptr) {
    handleCompileResult(false, "no compiler: " + compilerResult.error);
    return;
  }

  lastCompiledModTime = sourceFile.getLastModificationTime();
  compileInFlight = true;
  setDspState(processFn.load(std::memory_order_relaxed) != nullptr ? DspState::Recompiling
                                                                   : DspState::Compiling);

  // each compile gets subdirectory, dlopen never sees stale cached handle
  auto outputDir = buildRoot.getChildFile(juce::String(compileCount++));
  outputDir.createDirectory();

  // a superseded request may never call back, only latest generation counts
  const int generation = ++compileGeneration;

  compileService->compile(sourceFile, outputDir,
    [this, generation](CompileService::Result r) {
      if (generation != compileGeneration) {
        return;
      }

      compileInFlight = false;

      // user hit stop while this was building, discard the result
      if (!armed) {
        return;
      }

      if (!r.success) {
        handleCompileResult(false, r.compilerOutput);
        return;
      }

      juce::String loadError;
      auto newLib = DspLibrary::load(r.outputLib, loadError);

      if (newLib == nullptr) {
        handleCompileResult(false, "load failed: " + loadError);
        return;
      }

      if (currentSampleRate > 0.0) {
        newLib->prepare(currentSampleRate, currentBlockSize);
      }

      auto newFn     = newLib->processFn;
      auto oldLib    = std::move(activeDsp);
      activeDsp      = std::move(newLib);

      // atomic publish (audio thread picks up on next block)
      processFn.store(newFn, std::memory_order_release);

      if (oldLib != nullptr) {
        retireQueue.push_back(std::move(oldLib));
        scheduleRetireDrain();
      }

      handleCompileResult(true, r.compilerOutput);
    });
}

void QuillAudioProcessor::handleCompileResult(bool success, const juce::String& output) {
  // DBG compiles out in release builds
  juce::ignoreUnused(output);

  if (success) {
    setDspState(DspState::Running);
    return;
  }

  DBG("compile failed: " << output);

  // failed rebuild keeps the old dsp audible
  setDspState(processFn.load(std::memory_order_relaxed) != nullptr ? DspState::RunningError
                                                                   : DspState::Error);
}

void QuillAudioProcessor::stopDsp() {
  processFn.store(nullptr, std::memory_order_release);

  if (activeDsp != nullptr) {
    retireQueue.push_back(std::move(activeDsp));
    scheduleRetireDrain();
  }
}

void QuillAudioProcessor::scheduleRetireDrain() {
  // wait enough blocks for audio thread to be off the old function
  juce::Timer::callAfterDelay(200, [this] {
    juce::Array<juce::File> retiredDirs;
    for (auto& lib : retireQueue) {
      retiredDirs.add(lib->libraryFile.getParentDirectory());
    }

    // dlclose before deleting
    retireQueue.clear();

    for (auto& dir : retiredDirs) {
      dir.deleteRecursively();
    }
  });
}

juce::AudioProcessorEditor* QuillAudioProcessor::createEditor() {
  return new QuillAudioProcessorEditor (*this);
}

bool QuillAudioProcessor::hasEditor() const {
  return true;
}

const juce::String QuillAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool QuillAudioProcessor::acceptsMidi() const {
  return false;
}

bool QuillAudioProcessor::producesMidi() const {
  return false;
}

bool QuillAudioProcessor::isMidiEffect() const {
  return false;
}

double QuillAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int QuillAudioProcessor::getNumPrograms() {
  return 1;
}

int QuillAudioProcessor::getCurrentProgram() {
  return 0;
}

void QuillAudioProcessor::setCurrentProgram (int) {
}

const juce::String QuillAudioProcessor::getProgramName (int) {
  return {};
}

void QuillAudioProcessor::changeProgramName (int, const juce::String&) {
}

void QuillAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
  juce::XmlElement xml("QuillState");
  xml.setAttribute("sourceFile", sourceFile.getFullPathName());
  xml.setAttribute("armed", armed);
  copyXmlToBinary(xml, destData);
}

void QuillAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
  auto xml = getXmlFromBinary(data, sizeInBytes);
  if (xml == nullptr || !xml->hasTagName("QuillState")) {
    return;
  }

  auto path = xml->getStringAttribute("sourceFile");
  if (path.isEmpty()) {
    return;
  }

  setSourceFile(juce::File(path));

  // resume where session left off, missing files re-arm if they return
  if (xml->getBoolAttribute("armed")) {
    if (sourceFile.existsAsFile()) {
      arm();
    } else {
      armWhenFound = true;
    }
  }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new QuillAudioProcessor();
}

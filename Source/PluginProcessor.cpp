#include "PluginProcessor.h"
#include "PluginEditor.h"

QuillAudioProcessor::QuillAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {
  compilerResult = CompilerLocator::locate();

  if (compilerResult.found) {
    // navigate to shared using the compile-time source path
    auto sharedDir = juce::File(__FILE__).getParentDirectory().getSiblingFile("shared");
    compileService = std::make_unique<CompileService>(compilerResult.path, sharedDir);
  }
}

QuillAudioProcessor::~QuillAudioProcessor() {
  processFn.store(nullptr, std::memory_order_relaxed);
  compileService.reset();
  retireQueue.clear();
  activeDsp.reset();
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

void QuillAudioProcessor::compileAndLoad(const juce::File& sourceFile,
                                         std::function<void(bool, juce::String)> onResult) {
  if (compileService == nullptr) {
    if (onResult) {
      onResult(false, "no compiler: " + compilerResult.error);
    }
    return;
  }

  // each compile gets own subdirectory, dlopen never sees stale cached handle
  auto outputDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                     .getChildFile("quill_dsp")
                     .getChildFile(juce::String(compileCount++));
  outputDir.createDirectory();

  compileService->compile(sourceFile, outputDir,
    [this, onResult](CompileService::Result r) {
      if (!r.success) {
        if (onResult) { onResult(false, r.compilerOutput); }
        return;
      }

      juce::String loadError;
      auto newLib = DspLibrary::load(r.outputLib, loadError);

      if (newLib == nullptr) {
        if (onResult) { onResult(false, "load failed: " + loadError); }
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
      }

      // drain retire queue after enough blocks have passed
      juce::Timer::callAfterDelay(200, [this] { retireQueue.clear(); });

      if (onResult) { onResult(true, r.compilerOutput); }
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

void QuillAudioProcessor::getStateInformation (juce::MemoryBlock&) {
}

void QuillAudioProcessor::setStateInformation (const void*, int) {
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new QuillAudioProcessor();
}

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "CompilerLocator.h"
#include "CompileService.h"
#include "DspLibrary.h"
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

class QuillAudioProcessor : public juce::AudioProcessor,
                            public juce::ChangeBroadcaster,
                            private juce::Timer {
public:
  QuillAudioProcessor();
  ~QuillAudioProcessor() override;

  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

  void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram (int index) override;
  const juce::String getProgramName (int index) override;
  void changeProgramName (int index, const juce::String& newName) override;

  void getStateInformation (juce::MemoryBlock& destData) override;
  void setStateInformation (const void* data, int sizeInBytes) override;

  enum class DspState {
    Empty,        // no source selected
    Idle,         // file loaded, not armed
    Compiling,    // armed, nothing running yet
    Running,      // armed, dsp active
    Recompiling,  // armed, dsp active, rebuilding 
    RunningError, // armed, dsp active, last rebuild failed
    Error         // armed, nothing running, last compile failed
  };

  void setSourceFile(const juce::File& file);
  juce::File getSourceFile() const;

  DspState getDspState() const;
  bool isArmed() const;

  // arming compiles the source file and keeps it synced to saves on disk
  void arm();
  void disarm();

private:
  void timerCallback() override;

  void setDspState(DspState newState);
  void startCompile();
  void handleCompileResult(bool success, const juce::String& output);

  // silences the audio thread and retires the active dsp library
  void stopDsp();

  CompilerLocator::Result compilerResult;
  std::unique_ptr<CompileService> compileService;

  std::unique_ptr<DspLibrary> activeDsp;
  std::vector<std::unique_ptr<DspLibrary>> retireQueue;
  std::atomic<DspLibrary::ProcessFn> processFn { nullptr };

  juce::File sourceFile;
  DspState dspState { DspState::Empty };
  bool armed = false;

  // superseded compile callbacks are ignored by generation
  bool compileInFlight = false;
  int compileGeneration { 0 };

  juce::Time lastPolledModTime;
  juce::Time lastCompiledModTime;

  double currentSampleRate { 0.0 };
  int currentBlockSize { 0 };
  int compileCount { 0 };

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuillAudioProcessor)
};

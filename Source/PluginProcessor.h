#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "CompilerLocator.h"
#include "CompileService.h"
#include "DspLibrary.h"
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

class QuillAudioProcessor : public juce::AudioProcessor {
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

  void compileAndLoad(const juce::File& sourceFile,
                      std::function<void(bool, juce::String)> onResult);

  // silences the audio thread and retires the active dsp library
  void stopDsp();

private:
  CompilerLocator::Result compilerResult;
  std::unique_ptr<CompileService> compileService;

  std::unique_ptr<DspLibrary> activeDsp;
  std::vector<std::unique_ptr<DspLibrary>> retireQueue;
  std::atomic<DspLibrary::ProcessFn> processFn { nullptr };

  double currentSampleRate { 0.0 };
  int currentBlockSize { 0 };
  int compileCount { 0 };

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuillAudioProcessor)
};

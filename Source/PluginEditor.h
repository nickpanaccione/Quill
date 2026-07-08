#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class QuillAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit QuillAudioProcessorEditor (QuillAudioProcessor&);
  ~QuillAudioProcessorEditor() override;

  void paint (juce::Graphics&) override;
  void resized() override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuillAudioProcessorEditor)
};

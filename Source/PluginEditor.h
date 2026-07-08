#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <memory>

class QuillAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit QuillAudioProcessorEditor (QuillAudioProcessor&);
  ~QuillAudioProcessorEditor() override;

  void paint (juce::Graphics&) override;
  void resized() override;

private:
  void openFileAndCompile();

  QuillAudioProcessor& audioProcessor;

  juce::TextButton compileButton { "Compile..." };
  juce::Label statusLabel;
  std::unique_ptr<juce::FileChooser> fileChooser;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuillAudioProcessorEditor)
};

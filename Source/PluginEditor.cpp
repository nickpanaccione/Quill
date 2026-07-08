#include "PluginEditor.h"

namespace {
  const juce::Colour backgroundColour { 0xff1a1a1a };
  const juce::Colour textColour       { 0xff707070 };
}

QuillAudioProcessorEditor::QuillAudioProcessorEditor (QuillAudioProcessor& p)
  : AudioProcessorEditor (&p) {
  setSize (480, 320);
  setResizable (true, true);
}

QuillAudioProcessorEditor::~QuillAudioProcessorEditor() = default;

void QuillAudioProcessorEditor::paint (juce::Graphics& g) {
  g.fillAll (backgroundColour);

  g.setColour (textColour);
  g.setFont (juce::FontOptions (16.0f));
  g.drawText ("quill", getLocalBounds(), juce::Justification::centred);
}

void QuillAudioProcessorEditor::resized() {
}

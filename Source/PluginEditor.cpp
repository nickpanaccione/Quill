#include "PluginEditor.h"

namespace {
  const juce::Colour backgroundColour { 0xff1a1a1a };
  const juce::Colour textColour       { 0xff707070 };
}

QuillAudioProcessorEditor::QuillAudioProcessorEditor (QuillAudioProcessor& p)
  : AudioProcessorEditor (&p), audioProcessor (p) {
  setSize (480, 320);
  setResizable (true, true);

  addAndMakeVisible (compileButton);
  compileButton.onClick = [this] { openFileAndCompile(); };
  compileButton.setColour (juce::TextButton::buttonColourId,    juce::Colour { 0xff2a2a2a });
  compileButton.setColour (juce::TextButton::textColourOffId,   textColour);

  addAndMakeVisible (statusLabel);
  statusLabel.setText ("ready", juce::dontSendNotification);
  statusLabel.setColour (juce::Label::textColourId, textColour);
  statusLabel.setFont (juce::FontOptions (13.0f));
}

QuillAudioProcessorEditor::~QuillAudioProcessorEditor() = default;

void QuillAudioProcessorEditor::paint (juce::Graphics& g) {
  g.fillAll (backgroundColour);

  g.setColour (textColour);
  g.setFont (juce::FontOptions (16.0f));
  auto textArea = getLocalBounds().reduced (8).withTrimmedBottom (38);
  g.drawText ("quill", textArea, juce::Justification::centred);
}

void QuillAudioProcessorEditor::resized() {
  auto area   = getLocalBounds().reduced (8);
  auto bottom = area.removeFromBottom (24);

  compileButton.setBounds (bottom.removeFromLeft (90));
  bottom.removeFromLeft (8);
  statusLabel.setBounds (bottom);
}

void QuillAudioProcessorEditor::openFileAndCompile() {
  fileChooser = std::make_unique<juce::FileChooser> (
    "Select DSP source file",
    juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
    "*.cpp");

  auto flags = juce::FileBrowserComponent::openMode
             | juce::FileBrowserComponent::canSelectFiles;

  fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
    auto result = chooser.getResult();
    if (!result.existsAsFile()) {
      return;
    }

    statusLabel.setText ("compiling...", juce::dontSendNotification);
    compileButton.setEnabled (false);

    audioProcessor.compileAndLoad (result, [this] (bool success, juce::String message) {
      compileButton.setEnabled (true);
      if (success) {
        statusLabel.setText ("loaded", juce::dontSendNotification);
      } else {
        // truncate to fit the label
        statusLabel.setText ("error: " + message.substring (0, 60).trim(),
                             juce::dontSendNotification);
      }
    });
  });
}

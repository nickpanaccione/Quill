#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <functional>
#include <memory>
#include <vector>

class IconButton : public juce::Button {
public:
  explicit IconButton (const juce::String& name);

  void addIconState (const char* iconData, int iconSize,
                     const char* hoverData = nullptr, int hoverSize = 0);
  void setIconState (int newState);

private:
  void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

  struct Icons {
    std::unique_ptr<juce::Drawable> normal;
    std::unique_ptr<juce::Drawable> hover;
  };

  std::vector<Icons> iconStates;
  int iconState = 0;
};

// source picker (click to browse or drag .cpp file
class FolderButton : public IconButton, public juce::FileDragAndDropTarget {
public:
  FolderButton();

  std::function<void (juce::File)> onFileSelected;

private:
  enum IconState { Default = 0, Drag, Loaded };

  void clicked() override;
  bool isInterestedInFileDrag (const juce::StringArray& files) override;
  void fileDragEnter (const juce::StringArray&, int, int) override;
  void fileDragExit (const juce::StringArray&) override;
  void filesDropped (const juce::StringArray& files, int, int) override;
  void selectFile (const juce::File& file);

  bool isLoaded = false;
  std::unique_ptr<juce::FileChooser> fileChooser;
};

// cycles loading frames over run button while a compile in flight
class LoadingSpinner : public juce::Component, private juce::Timer {
public:
  LoadingSpinner();

  void start();
  void stop();

private:
  void paint (juce::Graphics&) override;
  void timerCallback() override;

  std::vector<std::unique_ptr<juce::Drawable>> frames;
  int frame = 0;
};

class QuillAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit QuillAudioProcessorEditor (QuillAudioProcessor&);
  ~QuillAudioProcessorEditor() override;

  void paint (juce::Graphics&) override;
  void resized() override;

private:
  void runClicked();
  void startCompile();

  QuillAudioProcessor& audioProcessor;

  IconButton runButton { "run" };
  FolderButton folderButton;
  LoadingSpinner loadingSpinner;

  juce::File currentSourceFile;
  bool dspRunning = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuillAudioProcessorEditor)
};

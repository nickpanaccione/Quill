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
  void setErrorBadge (bool shouldShow);

private:
  void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

  struct Icons {
    std::unique_ptr<juce::Drawable> normal;
    std::unique_ptr<juce::Drawable> hover;
  };

  std::vector<Icons> iconStates;
  int iconState = 0;
  bool errorBadge = false;
};

// source picker (click to browse or drag .cpp file
class FolderButton : public IconButton, public juce::FileDragAndDropTarget {
public:
  FolderButton();

  enum class FileStatus { none, loaded, missing };

  std::function<void (juce::File)> onFileSelected;
  void setFileStatus (FileStatus newStatus);

private:
  enum IconState { Default = 0, Drag, Loaded, NotFound };

  void clicked() override;
  bool isInterestedInFileDrag (const juce::StringArray& files) override;
  void fileDragEnter (const juce::StringArray&, int, int) override;
  void fileDragExit (const juce::StringArray&) override;
  void filesDropped (const juce::StringArray& files, int, int) override;
  void selectFile (const juce::File& file);
  int baseIcon() const;

  FileStatus fileStatus = FileStatus::none;
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

class QuillAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::ChangeListener {
public:
  explicit QuillAudioProcessorEditor (QuillAudioProcessor&);
  ~QuillAudioProcessorEditor() override;

  void paint (juce::Graphics&) override;
  void resized() override;

private:
  void changeListenerCallback (juce::ChangeBroadcaster*) override;
  void updateFromState();

  QuillAudioProcessor& audioProcessor;

  IconButton runButton { "run" };
  FolderButton folderButton;
  LoadingSpinner loadingSpinner;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuillAudioProcessorEditor)
};

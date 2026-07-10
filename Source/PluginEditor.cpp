#include "PluginEditor.h"
#include "BinaryData.h"

namespace {
  const juce::Colour backgroundColour { 0xff3c3836 };

  constexpr int windowWidth  = 390;
  constexpr int windowHeight = 195;
  constexpr int buttonSize   = 188;
  constexpr int padding      = 5;

  constexpr int spinnerFrameMs = 120;

  // run button icon states
  constexpr int runEmpty = 0;
  constexpr int runStart = 1;
  constexpr int runStop  = 2;
  constexpr int runError = 3;

  std::unique_ptr<juce::Drawable> loadIcon (const char* data, int size) {
    return data != nullptr ? juce::Drawable::createFromImageData (data, (size_t) size) : nullptr;
  }
}

// IconButton

IconButton::IconButton (const juce::String& name) : juce::Button (name) {
}

void IconButton::addIconState (const char* iconData, int iconSize,
                               const char* hoverData, int hoverSize) {
  iconStates.push_back ({ loadIcon (iconData, iconSize), loadIcon (hoverData, hoverSize) });
}

void IconButton::setIconState (int newState) {
  iconState = newState;
  repaint();
}

void IconButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool) {
  if (! juce::isPositiveAndBelow (iconState, (int) iconStates.size())) {
    return;
  }

  auto& icons = iconStates[(size_t) iconState];
  auto* icon = (shouldDrawButtonAsHighlighted && icons.hover != nullptr) ? icons.hover.get()
                                                                         : icons.normal.get();

  if (icon != nullptr) {
    icon->drawWithin (g, getLocalBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
  }
}

// FolderButton 

FolderButton::FolderButton() : IconButton ("folder") {
  addIconState (BinaryData::icon_folder_default_svg,      BinaryData::icon_folder_default_svgSize,
                BinaryData::icon_folder_hover_svg,        BinaryData::icon_folder_hover_svgSize);
  addIconState (BinaryData::icon_folder_drag_svg,         BinaryData::icon_folder_drag_svgSize);
  addIconState (BinaryData::icon_folder_loaded_svg,       BinaryData::icon_folder_loaded_svgSize,
                BinaryData::icon_folder_loaded_hover_svg, BinaryData::icon_folder_loaded_hover_svgSize);
}

void FolderButton::clicked() {
  fileChooser = std::make_unique<juce::FileChooser> (
    "Select DSP source file",
    juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
    "*.cpp");

  auto flags = juce::FileBrowserComponent::openMode
             | juce::FileBrowserComponent::canSelectFiles;

  fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser) {
    auto result = chooser.getResult();
    if (result.existsAsFile()) {
      selectFile (result);
    }
  });
}

bool FolderButton::isInterestedInFileDrag (const juce::StringArray& files) {
  if (!isEnabled()) {
    return false;
  }

  for (auto& f : files) {
    if (juce::File (f).hasFileExtension ("cpp")) {
      return true;
    }
  }
  return false;
}

void FolderButton::fileDragEnter (const juce::StringArray&, int, int) {
  setIconState (Drag);
}

void FolderButton::fileDragExit (const juce::StringArray&) {
  setIconState (isLoaded ? Loaded : Default);
}

void FolderButton::filesDropped (const juce::StringArray& files, int, int) {
  for (auto& f : files) {
    juce::File file (f);
    if (file.hasFileExtension ("cpp")) {
      selectFile (file);
      break;
    }
  }

  setIconState (isLoaded ? Loaded : Default);
}

void FolderButton::selectFile (const juce::File& file) {
  isLoaded = true;
  setIconState (Loaded);

  if (onFileSelected) {
    onFileSelected (file);
  }
}

// LoadingSpinner

LoadingSpinner::LoadingSpinner() {
  frames.push_back (loadIcon (BinaryData::icon_loading_1_svg, BinaryData::icon_loading_1_svgSize));
  frames.push_back (loadIcon (BinaryData::icon_loading_2_svg, BinaryData::icon_loading_2_svgSize));
  frames.push_back (loadIcon (BinaryData::icon_loading_3_svg, BinaryData::icon_loading_3_svgSize));

  setInterceptsMouseClicks (false, false);
  setVisible (false);
}

void LoadingSpinner::start() {
  frame = 0;
  setVisible (true);
  startTimer (spinnerFrameMs);
}

void LoadingSpinner::stop() {
  stopTimer();
  setVisible (false);
}

void LoadingSpinner::paint (juce::Graphics& g) {
  if (auto* f = frames[(size_t) frame].get()) {
    f->drawWithin (g, getLocalBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
  }
}

void LoadingSpinner::timerCallback() {
  frame = (frame + 1) % (int) frames.size();
  repaint();
}

// QuillAudioProcessorEditor

QuillAudioProcessorEditor::QuillAudioProcessorEditor (QuillAudioProcessor& p)
  : AudioProcessorEditor (&p), audioProcessor (p) {
  setSize (windowWidth, windowHeight);
  setResizable (false, false);

  runButton.addIconState (BinaryData::icon_run_empty_svg,       BinaryData::icon_run_empty_svgSize);
  runButton.addIconState (BinaryData::icon_run_start_svg,       BinaryData::icon_run_start_svgSize,
                          BinaryData::icon_run_start_hover_svg, BinaryData::icon_run_start_hover_svgSize);
  runButton.addIconState (BinaryData::icon_run_stop_svg,        BinaryData::icon_run_stop_svgSize,
                          BinaryData::icon_run_stop_hover_svg,  BinaryData::icon_run_stop_hover_svgSize);
  runButton.addIconState (BinaryData::icon_run_error_svg,       BinaryData::icon_run_error_svgSize,
                          BinaryData::icon_run_error_hover_svg, BinaryData::icon_run_error_hover_svgSize);
  runButton.setIconState (runEmpty);
  runButton.setEnabled (false);
  runButton.onClick = [this] { runClicked(); };
  addAndMakeVisible (runButton);

  folderButton.onFileSelected = [this] (juce::File file) {
    currentSourceFile = file;
    runButton.setEnabled (true);

    // fresh file clears error
    if (!dspRunning) {
      runButton.setIconState (runStart);
    }
  };
  addAndMakeVisible (folderButton);

  addChildComponent (loadingSpinner);
}

QuillAudioProcessorEditor::~QuillAudioProcessorEditor() = default;

void QuillAudioProcessorEditor::paint (juce::Graphics& g) {
  g.fillAll (backgroundColour);
}

void QuillAudioProcessorEditor::resized() {
  const int top = (getHeight() - buttonSize) / 2;

  runButton.setBounds (padding, top, buttonSize, buttonSize);
  folderButton.setBounds (getWidth() - padding - buttonSize, top, buttonSize, buttonSize);
  loadingSpinner.setBounds (runButton.getBounds());
}

void QuillAudioProcessorEditor::runClicked() {
  if (dspRunning) {
    audioProcessor.stopDsp();
    dspRunning = false;
    runButton.setIconState (runStart);
    return;
  }

  startCompile();
}

void QuillAudioProcessorEditor::startCompile() {
  if (!currentSourceFile.existsAsFile()) {
    return;
  }

  runButton.setEnabled (false);
  folderButton.setEnabled (false);
  loadingSpinner.start();

  audioProcessor.compileAndLoad (currentSourceFile, [this] (bool success, juce::String message) {
    loadingSpinner.stop();
    folderButton.setEnabled (true);
    runButton.setEnabled (true);

    // DBG compiles out in release builds
    juce::ignoreUnused (message);
    if (!success) {
      DBG ("compile failed: " << message);
    }

    dspRunning = success;
    runButton.setIconState (dspRunning ? runStop : runError);
  });
}

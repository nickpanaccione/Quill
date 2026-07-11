#include "PluginEditor.h"
#include "BinaryData.h"

namespace {
  const juce::Colour backgroundColour { 0xff3c3836 };
  const juce::Colour errorBadgeColour { 0xfffb4934 };

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

void IconButton::setErrorBadge (bool shouldShow) {
  if (errorBadge != shouldShow) {
    errorBadge = shouldShow;
    repaint();
  }
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

  // last rebuild failed but the old dsp is still audible
  if (errorBadge) {
    auto bounds = getLocalBounds().toFloat();
    auto diameter = bounds.getWidth() * 0.1f;
    g.setColour (errorBadgeColour);
    g.fillEllipse (bounds.getRight() - diameter * 1.8f, bounds.getY() + diameter * 0.8f,
                   diameter, diameter);
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
  setLoaded (true);

  if (onFileSelected) {
    onFileSelected (file);
  }
}

void FolderButton::setLoaded (bool shouldBeLoaded) {
  isLoaded = shouldBeLoaded;
  setIconState (isLoaded ? Loaded : Default);
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
  runButton.onClick = [this] {
    if (audioProcessor.isArmed()) {
      audioProcessor.disarm();
    } else {
      audioProcessor.arm();
    }
  };
  addAndMakeVisible (runButton);

  folderButton.onFileSelected = [this] (juce::File file) {
    audioProcessor.setSourceFile (file);
  };
  addAndMakeVisible (folderButton);

  addChildComponent (loadingSpinner);

  // sync with processor, keeps running while the editor is closed
  audioProcessor.addChangeListener (this);
  folderButton.setLoaded (audioProcessor.getSourceFile().existsAsFile());
  updateFromState();
}

QuillAudioProcessorEditor::~QuillAudioProcessorEditor() {
  audioProcessor.removeChangeListener (this);
}

void QuillAudioProcessorEditor::paint (juce::Graphics& g) {
  g.fillAll (backgroundColour);
}

void QuillAudioProcessorEditor::resized() {
  const int top = (getHeight() - buttonSize) / 2;

  runButton.setBounds (padding, top, buttonSize, buttonSize);
  folderButton.setBounds (getWidth() - padding - buttonSize, top, buttonSize, buttonSize);
  loadingSpinner.setBounds (runButton.getBounds());
}

void QuillAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*) {
  updateFromState();
}

void QuillAudioProcessorEditor::updateFromState() {
  using State = QuillAudioProcessor::DspState;
  const auto state = audioProcessor.getDspState();

  const bool compiling = (state == State::Compiling || state == State::Recompiling);
  if (compiling && !loadingSpinner.isVisible()) {
    loadingSpinner.start();
  } else if (!compiling) {
    loadingSpinner.stop();
  }

  runButton.setEnabled (state != State::Empty);
  runButton.setErrorBadge (state == State::RunningError);

  switch (state) {
    case State::Empty:        runButton.setIconState (runEmpty); break;
    case State::Idle:
    case State::Compiling:    runButton.setIconState (runStart); break;
    case State::Running:
    case State::Recompiling:
    case State::RunningError: runButton.setIconState (runStop);  break;
    case State::Error:        runButton.setIconState (runError); break;
  }
}

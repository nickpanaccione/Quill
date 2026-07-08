#include "PluginProcessor.h"
#include "PluginEditor.h"

QuillAudioProcessor::QuillAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {
}

QuillAudioProcessor::~QuillAudioProcessor() = default;

void QuillAudioProcessor::prepareToPlay (double, int) {
  // No DSP yet 
}

void QuillAudioProcessor::releaseResources() {
}

bool QuillAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() 
    && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }

  return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void QuillAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;

  juce::ignoreUnused (buffer);
}

juce::AudioProcessorEditor* QuillAudioProcessor::createEditor() {
  return new QuillAudioProcessorEditor (*this);
}

bool QuillAudioProcessor::hasEditor() const {
  return true;
}

const juce::String QuillAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool QuillAudioProcessor::acceptsMidi() const {
  return false;
}

bool QuillAudioProcessor::producesMidi() const {
  return false;
}

bool QuillAudioProcessor::isMidiEffect() const {
  return false;
}

double QuillAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int QuillAudioProcessor::getNumPrograms() {
  return 1;
}

int QuillAudioProcessor::getCurrentProgram() {
  return 0;
}

void QuillAudioProcessor::setCurrentProgram (int) {
}

const juce::String QuillAudioProcessor::getProgramName (int) {
  return {};
}

void QuillAudioProcessor::changeProgramName (int, const juce::String&) {
}

void QuillAudioProcessor::getStateInformation (juce::MemoryBlock&) {
}

void QuillAudioProcessor::setStateInformation (const void*, int) {
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new QuillAudioProcessor();
}

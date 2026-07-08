#pragma once
#include <juce_core/juce_core.h>

class CompilerLocator {
public:
  struct Result {
    bool found = false;
    juce::File path;
    juce::String version;
    juce::String error;
  };

  static Result locate();

private:
  static juce::File findViaXcrun();
  static bool validate(const juce::File& compiler, juce::String& versionOut);
};

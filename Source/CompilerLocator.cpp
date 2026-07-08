#include "CompilerLocator.h"

static const char* kCandidatePaths[] = {
  "/usr/bin/clang++",
  "/usr/local/bin/clang++",    // homebrew intel
  "/opt/homebrew/bin/clang++", // homebrew apple silicon
  nullptr
};

CompilerLocator::Result CompilerLocator::locate() {
  Result result;

  // resolves clang++ through xcode or command line tools
  auto xcrunPath = findViaXcrun();
  if (xcrunPath.existsAsFile()) {
    if (validate(xcrunPath, result.version)) {
      result.found = true;
      result.path = xcrunPath;
      return result;
    }
  }

  // fall back to fixed paths
  for (int i = 0; kCandidatePaths[i] != nullptr; ++i) {
    juce::File candidate(kCandidatePaths[i]);
    if (candidate.existsAsFile()) {
      if (validate(candidate, result.version)) {
        result.found = true;
        result.path = candidate;
        return result;
      }
    }
  }

  result.error = "no valid clang++ found on this system";
  return result;
}

juce::File CompilerLocator::findViaXcrun() {
  juce::ChildProcess proc;
  if (!proc.start(juce::StringArray { "xcrun", "--find", "clang++" })) {
    return {};
  }
  auto path = proc.readAllProcessOutput().trim();
  proc.waitForProcessToFinish(5000);
  return juce::File(path);
}

bool CompilerLocator::validate(const juce::File& compiler, juce::String& versionOut) {
  juce::ChildProcess proc;
  if (!proc.start(juce::StringArray { compiler.getFullPathName(), "--version" })) {
    return false;
  }
  versionOut = proc.readAllProcessOutput().trim();
  proc.waitForProcessToFinish(5000);
  return versionOut.isNotEmpty();
}

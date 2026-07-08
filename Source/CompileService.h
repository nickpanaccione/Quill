#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <memory>

class CompileService : public juce::Thread {
public:
  struct Result {
    bool success = false;
    juce::File outputLib;
    juce::String compilerOutput;
  };

  using Callback = std::function<void(Result)>;

  CompileService(juce::File compilerPath, juce::File sharedIncludeDir);
  ~CompileService() override;

  // replaces pending request that hasn't started yet
  void compile(juce::File sourceFile, juce::File outputDir, Callback callback);

private:
  void run() override;

  struct Request {
    juce::File sourceFile;
    juce::File outputDir;
    Callback callback;
  };

  juce::File compiler;
  juce::File sharedIncludeDir;

  juce::CriticalSection lock;
  std::unique_ptr<Request> pending;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompileService)
};

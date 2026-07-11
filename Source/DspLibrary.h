#pragma once

#include <juce_core/juce_core.h>
#include <dlfcn.h>
#include <atomic>

class DspLibrary {
public:
  using PrepareFn = void (*)(double, int);
  using ProcessFn = void (*)(const float*, float*, int);

  static std::unique_ptr<DspLibrary> load(const juce::File& path, juce::String& error);

  ~DspLibrary();

  void prepare(double sampleRate, int blockSize) const;

  PrepareFn prepareFn = nullptr;
  ProcessFn processFn = nullptr;
  juce::File libraryFile;

private:
  DspLibrary() = default;
  void* handle = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspLibrary)
};

static_assert(std::atomic<DspLibrary::ProcessFn>::is_always_lock_free,
              "ProcessFn atomic must be lock-free on the audio thread");

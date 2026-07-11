#include "DspLibrary.h"

std::unique_ptr<DspLibrary> DspLibrary::load(const juce::File& path, juce::String& error) {
  auto lib = std::unique_ptr<DspLibrary>(new DspLibrary());

  // RTLD_NOW: resolve all symbols at load time, errors surface immediately
  // RTLD_LOCAL: keep symbols out of global namespace
  lib->handle = dlopen(path.getFullPathName().toRawUTF8(), RTLD_NOW | RTLD_LOCAL);
  if (lib->handle == nullptr) {
    error = juce::String::fromUTF8(dlerror());
    return nullptr;
  }

  lib->prepareFn = reinterpret_cast<PrepareFn>(dlsym(lib->handle, "dsp_prepare"));
  lib->processFn = reinterpret_cast<ProcessFn>(dlsym(lib->handle, "dsp_process"));

  if (lib->prepareFn == nullptr || lib->processFn == nullptr) {
    error = "dylib is missing dsp_prepare or dsp_process";
    dlclose(lib->handle);
    lib->handle = nullptr;
    return nullptr;
  }

  lib->libraryFile = path;
  return lib;
}

DspLibrary::~DspLibrary() {
  if (handle != nullptr) {
    dlclose(handle);
  }
}

void DspLibrary::prepare(double sampleRate, int blockSize) const {
  if (prepareFn != nullptr) {
    prepareFn(sampleRate, blockSize);
  }
}

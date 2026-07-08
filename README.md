# Quill

C++ DSP prototyping.

Quill is an audio plugin (VST3/AU) that lets you write, compile, and hot-swap
C++ DSP code while it's loaded in a live signal chain.

## Status

Early scaffold. Currently just a passthrough audio plugin shell built with
JUCE.

## Build

Requires CMake and a C++20 compiler (Xcode Command Line Tools on macOS).

```bash
git clone --recurse-submodules https://github.com/nickpanaccione/Quill 
cd Quill
cmake --build build
```

This produces VST3, AU, and Standalone targets

## License

GPLv3 — see [LICENSE](./LICENSE).

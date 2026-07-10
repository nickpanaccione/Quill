# Quill
[![Build](https://github.com/nickpanaccione/Quill/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/nickpanaccione/Quill/actions/workflows/build.yml)

C++ DSP prototyping.

## Description

Quill is an audio plugin (VST3/AU) that lets you write, compile,
and hot-swap C++ DSP code while it's loaded in a live signal chain. Edit a
`.cpp` file, compile it in the background, and swap the DSP
function into the audio thread without restarting the plugin or leaving
your DAW session. Built with JUCE.

## Getting Started

### Dependencies

* CMake 3.22+
* A C++20 compiler on macOS, the Xcode Command Line Tools (`clang++` via
  `xcrun`)
* JUCE (included as a git submodule)

### Installing

```bash
git clone --recurse-submodules https://github.com/nickpanaccione/Quill
cd Quill
```

### Executing program

Configure and build with CMake:

```bash
cmake -B build -G Ninja
cmake --build build
```

This produces AU, VST3, and Standalone targets. 

## License

This project is licensed under the GPLv3 License - see the
[LICENSE](./LICENSE) file for details.

## Acknowledgments

* Built with [JUCE](https://juce.com/)

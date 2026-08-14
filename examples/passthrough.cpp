// passthrough.cpp
// copies input straight to output, no processing.

#include "dsp_interface.h"

#include <cstring>

void dsp_prepare(double /*sampleRate*/, int /*blockSize*/) {
}

void dsp_process(const float* input, float* output, int numSamples) {
  memcpy(output, input, size_t(numSamples) * sizeof(float));
}

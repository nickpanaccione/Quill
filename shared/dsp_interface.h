#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// user dsp code must define these
void dsp_prepare(double sampleRate, int blockSize);
void dsp_process(const float* input, float* output, int numSamples);

#ifdef __cplusplus
}
#endif

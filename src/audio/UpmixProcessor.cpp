// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// UpmixProcessor.cpp

#include "UpmixProcessor.h"

namespace virtuoso {

void UpmixProcessor::process(const juce::AudioBuffer<float> &src,
                             juce::AudioBuffer<float> &dst, SourceFormat format,
                             int numSamples, float headroomGain) noexcept {
  jassert(dst.getNumChannels() == 8);
  jassert(numSamples <= dst.getNumSamples());

  if (format == SourceFormat::Surround71) {
    // 7.1 input: direct copy with headroom
    for (int ch = 0; ch < 8; ++ch) {
      if (ch < src.getNumChannels()) {
        juce::FloatVectorOperations::copyWithMultiply(dst.getWritePointer(ch),
                                                      src.getReadPointer(ch),
                                                      headroomGain, numSamples);
      } else {
        juce::FloatVectorOperations::clear(dst.getWritePointer(ch), numSamples);
      }
    }
    return;
  }

  if (format == SourceFormat::Surround51) {
    // 5.1 → 7.1 matrix (6 input channels → 8 output channels)
    for (int outCh = 0; outCh < 8; ++outCh) {
      float *out = dst.getWritePointer(outCh);
      juce::FloatVectorOperations::clear(out, numSamples);

      for (int inCh = 0; inCh < 6 && inCh < src.getNumChannels(); ++inCh) {
        const float c = kSurround51Matrix[outCh][inCh];
        if (c == 0.0f)
          continue;
        const float *in = src.getReadPointer(inCh);
        juce::FloatVectorOperations::addWithMultiply(out, in, c * headroomGain,
                                                     numSamples);
      }
    }
    return;
  }

  // Stereo → 7.1 matrix (2 input channels → 8 output channels)
  for (int outCh = 0; outCh < 8; ++outCh) {
    float *out = dst.getWritePointer(outCh);
    juce::FloatVectorOperations::clear(out, numSamples);

    for (int inCh = 0; inCh < 2 && inCh < src.getNumChannels(); ++inCh) {
      const float c = kStereoMatrix[outCh][inCh];
      if (c == 0.0f)
        continue;
      const float *in = src.getReadPointer(inCh);
      juce::FloatVectorOperations::addWithMultiply(out, in, c * headroomGain,
                                                   numSamples);
    }
  }
}

const UpmixProcessor::Matrix71 &UpmixProcessor::stereoMatrix() noexcept {
  static Matrix71 m;
  static bool init = false;
  if (!init) {
    for (int i = 0; i < 8; ++i)
      m.c[i] = {kStereoMatrix[i][0], kStereoMatrix[i][1]};
    init = true;
  }
  return m;
}

} // namespace virtuoso

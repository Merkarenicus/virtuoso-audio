// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ResamplerProcessor.h
// Internal resampler: converts virtual device sample rate → engine sample rate.
// Uses libsamplerate (SRC_SINC_BEST_QUALITY) for high-quality conversion.
// Operates on interleaved float data (JUCE AudioBuffer → interleaved → SRC →
// deinterleaved).

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <samplerate.h>
#include <vector>

namespace virtuoso {

class ResamplerProcessor {
public:
  ResamplerProcessor();
  ~ResamplerProcessor();

  ResamplerProcessor(const ResamplerProcessor &) = delete;
  ResamplerProcessor &operator=(const ResamplerProcessor &) = delete;

  // Call when either sample rate changes.
  // numChannels: number of audio channels to resample (typically 8 for 7.1)
  bool prepare(double inputSampleRate, double outputSampleRate,
               int numChannels);

  // Reset/flush SRC internal state (call before offline render)
  void reset();

  // Resample input → output.
  // Returns actual number of output samples written.
  int process(const juce::AudioBuffer<float> &input,
              juce::AudioBuffer<float> &output, int inputSamples) noexcept;

  bool isPassthrough() const noexcept { return m_passthrough; }
  double getRatio() const noexcept { return m_ratio; }

private:
  SRC_STATE *m_src{nullptr};
  int m_numChannels{1};
  double m_ratio{1.0};
  bool m_passthrough{true};

  // Interleaved scratch buffers
  std::vector<float> m_interleavedIn;
  std::vector<float> m_interleavedOut;
};

} // namespace virtuoso

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ResamplerProcessor.cpp

#include "ResamplerProcessor.h"
#include <cassert>

namespace virtuoso {

ResamplerProcessor::ResamplerProcessor() = default;

ResamplerProcessor::~ResamplerProcessor() {
  if (m_src)
    src_delete(m_src);
}

bool ResamplerProcessor::prepare(double inputSampleRate,
                                 double outputSampleRate, int numChannels) {
  m_numChannels = numChannels;
  m_ratio = outputSampleRate / inputSampleRate;
  m_passthrough = (std::abs(m_ratio - 1.0) < 1e-6);

  if (m_src) {
    src_delete(m_src);
    m_src = nullptr;
  }

  if (!m_passthrough) {
    int error = 0;
    m_src = src_new(SRC_SINC_BEST_QUALITY, numChannels, &error);
    if (!m_src || error != 0) {
      m_passthrough = true;
      return false;
    }
    src_set_ratio(m_src, m_ratio);
  }
  return true;
}

void ResamplerProcessor::reset() {
  if (m_src)
    src_reset(m_src);
}

int ResamplerProcessor::process(const juce::AudioBuffer<float> &input,
                                juce::AudioBuffer<float> &output,
                                int inputSamples) noexcept {
  if (m_passthrough) {
    int n = std::min(inputSamples, output.getNumSamples());
    for (int ch = 0; ch < m_numChannels && ch < input.getNumChannels(); ++ch)
      juce::FloatVectorOperations::copy(output.getWritePointer(ch),
                                        input.getReadPointer(ch), n);
    return n;
  }

  // Interleave input
  const int totalIn = inputSamples * m_numChannels;
  m_interleavedIn.resize(static_cast<size_t>(totalIn));
  for (int i = 0; i < inputSamples; ++i)
    for (int ch = 0; ch < m_numChannels; ++ch)
      m_interleavedIn[static_cast<size_t>(i * m_numChannels + ch)] =
          (ch < input.getNumChannels()) ? input.getReadPointer(ch)[i] : 0.0f;

  const int maxOut = static_cast<int>(std::ceil(m_ratio * inputSamples)) + 4;
  m_interleavedOut.resize(static_cast<size_t>(maxOut * m_numChannels));

  SRC_DATA sd;
  sd.data_in = m_interleavedIn.data();
  sd.input_frames = inputSamples;
  sd.data_out = m_interleavedOut.data();
  sd.output_frames = maxOut;
  sd.src_ratio = m_ratio;
  sd.end_of_input = 0;

  if (src_process(m_src, &sd) != 0)
    return 0;

  const int outSamples = static_cast<int>(sd.output_frames_gen);
  // De-interleave to output buffer
  for (int i = 0; i < outSamples && i < output.getNumSamples(); ++i)
    for (int ch = 0; ch < m_numChannels && ch < output.getNumChannels(); ++ch)
      output.getWritePointer(ch)[i] =
          m_interleavedOut[static_cast<size_t>(i * m_numChannels + ch)];

  return outSamples;
}

} // namespace virtuoso

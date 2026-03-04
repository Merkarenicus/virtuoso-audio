// SPDX-License-Identifier: MIT
#include "HrirResampler.h"
namespace virtuoso {

std::expected<juce::AudioBuffer<float>, std::string>
HrirResampler::resample(const juce::AudioBuffer<float> &ir, double sourceSr,
                        double targetSr) {
  if (std::abs(sourceSr - targetSr) < 0.5)
    return ir; // Passthrough

  const int inLen = ir.getNumSamples();
  const int outLen =
      static_cast<int>(std::ceil(inLen * targetSr / sourceSr)) + 4;
  juce::AudioBuffer<float> out(1, outLen);

  ResamplerProcessor proc;
  if (!proc.prepare(sourceSr, targetSr, 1))
    return std::unexpected("Failed to initialize resampler for IR");

  int written = proc.process(ir, out, inLen);
  out.setSize(1, written, true, true, true);
  return out;
}

} // namespace virtuoso

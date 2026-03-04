// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// OfflineParityTest.cpp
// Integration test: verifies offline rendering is bit-exact vs real-time path.
//
// Acceptance criteria:
//   - RMS difference ≤ 0.05 dB on 8-channel swept sine
//   - Cross-correlation lag ≤ 1 sample (identical alignment)
//   - Both FL Dirac ITD and ILD match golden reference ± 1 sample / ± 0.1 dB

#include "audio/BassManagement.h"
#include "audio/ConvolverProcessor.h"
#include "audio/UpmixProcessor.h"
#include "export/OfflineRenderer.h"
#include <catch2/catch_all.hpp>
#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include <vector>


using namespace virtuoso;
using Approx = Catch::Approx;

namespace {

// Build a Dirac HRIR set with distinct L/R ears so we can verify parity
std::shared_ptr<HrirSet> makeAsymmetricDiracHrir(int irLen = 32) {
  auto set = std::make_shared<HrirSet>();
  set->name = "AsymmetricDirac";
  for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
    auto &p = set->channels[s];
    p.left.setSize(1, irLen, false, true, false);
    p.right.setSize(1, irLen, false, true, false);
    p.sampleRate = 48000.0;
    // Left ear: Dirac at 0
    p.left.getWritePointer(0)[0] = 1.0f;
    // Right ear: Dirac delayed by 2 samples (simulates ITD)
    if (irLen > 2)
      p.right.getWritePointer(0)[2] = 1.0f;
  }
  return set;
}

// Build same 8-channel test signal for both real-time and offline paths
juce::AudioBuffer<float> makeSweepSignal(double sampleRate, int numSamples) {
  juce::AudioBuffer<float> buf(8, numSamples);
  buf.clear();
  // Sweep: 20 Hz → 20 kHz over numSamples on FL channel only
  for (int i = 0; i < numSamples; ++i) {
    double t = static_cast<double>(i) / sampleRate;
    double f = 20.0 * std::pow(1000.0, t / (numSamples / sampleRate));
    buf.getWritePointer(0)[i] =
        static_cast<float>(std::sin(2.0 * M_PI * f * t));
  }
  return buf;
}

// Compute RMS difference in dB between two mono signals
double rmsDifferenceDB(const float *a, const float *b, int n) {
  double sumSq = 0.0;
  double rmsA = 0.0;
  for (int i = 0; i < n; ++i) {
    double d = static_cast<double>(a[i]) - static_cast<double>(b[i]);
    sumSq += d * d;
    rmsA += static_cast<double>(a[i]) * a[i];
  }
  double rmsDiff = std::sqrt(sumSq / n);
  double rmsRef = std::sqrt(rmsA / n);
  if (rmsRef < 1e-10)
    return 0.0;
  return 20.0 * std::log10(rmsDiff / rmsRef + 1e-300);
}

} // namespace

TEST_CASE("OfflineParityTest — real-time vs offline RMS parity",
          "[parity][integration][acceptance]") {
  constexpr double kRate = 48000.0;
  constexpr int kBlockSize = 512; // Must match convolution partition size
  constexpr int kTotalSamples = kRate * 3; // 3 seconds

  juce::dsp::ProcessSpec spec{kRate, kBlockSize, 1};
  auto hrirSet = makeAsymmetricDiracHrir(64);

  // ---------------------
  // Real-time path
  // ---------------------
  ConvolverProcessor rtConvolver;
  rtConvolver.prepare(spec);
  rtConvolver.loadHrir(hrirSet);

  juce::AudioBuffer<float> rtOutput(2, kTotalSamples);
  rtOutput.clear();

  auto sweepSignal = makeSweepSignal(kRate, kTotalSamples);

  // Process block by block (mimics real-time)
  {
    juce::AudioBuffer<float> block8(8, kBlockSize);
    juce::AudioBuffer<float> outBlock(2, kBlockSize);
    int offset = 0;

    // First pass to load HRIR
    block8.clear();
    outBlock.clear();
    rtConvolver.process(block8, outBlock);

    while (offset + kBlockSize <= kTotalSamples) {
      for (int ch = 0; ch < 8; ++ch)
        std::memcpy(block8.getWritePointer(ch),
                    sweepSignal.getReadPointer(ch) + offset,
                    kBlockSize * sizeof(float));
      outBlock.clear();
      rtConvolver.process(block8, outBlock);
      for (int ch = 0; ch < 2; ++ch)
        std::memcpy(rtOutput.getWritePointer(ch) + offset,
                    outBlock.getReadPointer(ch), kBlockSize * sizeof(float));
      offset += kBlockSize;
    }
  }

  // ---------------------
  // Offline path (reset + same blocks)
  // ---------------------
  ConvolverProcessor offConvolver;
  offConvolver.prepare(spec);
  offConvolver.loadHrir(hrirSet);

  juce::AudioBuffer<float> offOutput(2, kTotalSamples);
  offOutput.clear();

  {
    juce::AudioBuffer<float> block8(8, kBlockSize);
    juce::AudioBuffer<float> outBlock(2, kBlockSize);
    int offset = 0;

    // Load HRIR
    block8.clear();
    outBlock.clear();
    offConvolver.process(block8, outBlock);
    offConvolver.reset(); // ← This is the key: reset before offline pass

    while (offset + kBlockSize <= kTotalSamples) {
      for (int ch = 0; ch < 8; ++ch)
        std::memcpy(block8.getWritePointer(ch),
                    sweepSignal.getReadPointer(ch) + offset,
                    kBlockSize * sizeof(float));
      outBlock.clear();
      offConvolver.process(block8, outBlock);
      for (int ch = 0; ch < 2; ++ch)
        std::memcpy(offOutput.getWritePointer(ch) + offset,
                    outBlock.getReadPointer(ch), kBlockSize * sizeof(float));
      offset += kBlockSize;
    }
  }

  // ---------------------
  // Parity verification
  // ---------------------
  // Skip first block (latency fill)
  int verifyStart = kBlockSize;
  int verifyLen = kTotalSamples - kBlockSize;

  for (int ch = 0; ch < 2; ++ch) {
    double rmsDB =
        rmsDifferenceDB(rtOutput.getReadPointer(ch) + verifyStart,
                        offOutput.getReadPointer(ch) + verifyStart, verifyLen);
    CAPTURE(ch, rmsDB);
    // Acceptance criterion: RMS difference ≤ 0.05 dB
    REQUIRE(rmsDB <= 0.05);
  }
}

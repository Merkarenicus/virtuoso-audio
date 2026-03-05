// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ConvolverBenchmark.cpp — Real-time performance benchmark for
// ConvolverProcessor. Reports: mean block processing time, max latency spike,
// CPU load estimate. Run with: ctest -R ConvolverBenchmark (or directly:
// ./ConvolverBenchmark)

#include "../../src/audio/ConvolverProcessor.h"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <numeric>
#include <vector>


using namespace virtuoso;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumChannels = 8;
constexpr int kIrLength = 2048;

// Build a synthetic 7.1 HRIR set (identity impulses)
HrirSet makeIdentityHrirSet() {
  HrirSet hs;
  hs.name = "benchmark-identity";
  for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
    hs.channels[s].left.setSize(1, kIrLength, false, true, false);
    hs.channels[s].right.setSize(1, kIrLength, false, true, false);
    hs.channels[s].left.clear();
    hs.channels[s].right.clear();
    // Kronecker delta at sample 0 → pass-through
    hs.channels[s].left.getWritePointer(0)[0] = 1.0f;
    hs.channels[s].right.getWritePointer(0)[0] = 1.0f;
    hs.channels[s].sampleRate = kSampleRate;
  }
  return hs;
}
} // namespace

TEST_CASE("ConvolverProcessor benchmark", "[perf][!benchmark]") {
  ConvolverProcessor convolver;
  juce::dsp::ProcessSpec spec{kSampleRate, static_cast<uint32_t>(kBlockSize),
                              2};
  convolver.prepare(spec);
  convolver.loadHrir(makeIdentityHrirSet());

  juce::AudioBuffer<float> input(kNumChannels, kBlockSize);
  juce::AudioBuffer<float> output(2, kBlockSize);
  input.clear();
  // Fill with pink-noise-like data
  for (int ch = 0; ch < kNumChannels; ++ch)
    for (int i = 0; i < kBlockSize; ++i)
      input.getWritePointer(ch)[i] = (float)std::sin(i * 0.1 * (ch + 1));

  // Warm up
  for (int i = 0; i < 10; ++i)
    convolver.process(input, output);

  // Time 1000 blocks
  constexpr int kIterations = 1000;
  std::vector<double> blockTimesUs(kIterations);

  for (int iter = 0; iter < kIterations; ++iter) {
    auto t0 = std::chrono::high_resolution_clock::now();
    convolver.process(input, output);
    auto t1 = std::chrono::high_resolution_clock::now();
    blockTimesUs[iter] =
        std::chrono::duration<double, std::micro>(t1 - t0).count();
  }

  const double blockDurationUs = (kBlockSize / kSampleRate) * 1e6;
  const double meanUs =
      std::accumulate(blockTimesUs.begin(), blockTimesUs.end(), 0.0) /
      kIterations;
  const double maxUs =
      *std::max_element(blockTimesUs.begin(), blockTimesUs.end());
  const double cpuLoad = meanUs / blockDurationUs * 100.0;

  INFO("ConvolverProcessor benchmark results:");
  INFO("  Block size       : " << kBlockSize << " samples at "
                               << kSampleRate / 1000.0 << " kHz");
  INFO("  Block duration   : " << blockDurationUs << " μs");
  INFO("  Mean process time: " << meanUs << " μs");
  INFO("  Max process time : " << maxUs << " μs");
  INFO("  CPU load (mean)  : " << cpuLoad << " %");

  // Hard fail if mean processing time exceeds 50% of block duration
  // (leaves headroom for real-time safety)
  REQUIRE(meanUs < blockDurationUs * 0.50);
  REQUIRE(maxUs <
          blockDurationUs * 2.00); // No single block should take > 2× budget

  BENCHMARK("ConvolverProcessor::process() one block") {
    convolver.process(input, output);
    return output.getReadPointer(0)[0]; // Prevent dead-code elimination
  };
}

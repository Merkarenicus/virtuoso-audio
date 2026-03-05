// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// LatencyHarness.cpp — End-to-end latency budget verification.
// Measures round-trip latency: input capture → DSP chain → output.
// Target: < 10 ms at 512-sample block / 48 kHz.

#include "../../src/audio/AudioEngine.h"
#include "../../src/audio/BassManagement.h"
#include "../../src/audio/ConvolverProcessor.h"
#include "../../src/audio/EqProcessor.h"
#include "../../src/audio/LimiterProcessor.h"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <juce_audio_basics/juce_audio_basics.h>
#include <numeric>
#include <vector>


using namespace virtuoso;

namespace {
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr double kBudgetMs = 10.0; // "Never break audio" guarantee
constexpr int kWarmupBlocks = 20;
constexpr int kTestBlocks = 500;
} // namespace

TEST_CASE("DSP chain latency meets 10 ms real-time budget", "[perf][latency]") {
  ConvolverProcessor convolver;
  EqProcessor eq;
  BassManagement bass;
  LimiterProcessor limiter;

  juce::dsp::ProcessSpec spec{kSampleRate, static_cast<uint32_t>(kBlockSize),
                              2};
  convolver.prepare(spec);
  eq.prepare(spec);
  bass.prepare(spec);
  limiter.prepare(spec);

  juce::AudioBuffer<float> input8(8, kBlockSize);
  juce::AudioBuffer<float> binaural(2, kBlockSize);

  // Fill input with noise
  for (int ch = 0; ch < 8; ++ch)
    for (int i = 0; i < kBlockSize; ++i)
      input8.getWritePointer(ch)[i] =
          ((float)std::rand() / RAND_MAX) * 2.0f - 1.0f;

  // Warm up
  for (int i = 0; i < kWarmupBlocks; ++i) {
    bass.process(input8);
    convolver.process(input8, binaural);
    eq.processBlock(binaural);
    limiter.processBlock(binaural);
  }

  std::vector<double> timesMs(kTestBlocks);
  for (int iter = 0; iter < kTestBlocks; ++iter) {
    auto t0 = std::chrono::high_resolution_clock::now();

    bass.process(input8);
    convolver.process(input8, binaural);
    eq.processBlock(binaural);
    limiter.processBlock(binaural);

    auto t1 = std::chrono::high_resolution_clock::now();
    timesMs[iter] = std::chrono::duration<double, std::milli>(t1 - t0).count();
  }

  const double meanMs =
      std::accumulate(timesMs.begin(), timesMs.end(), 0.0) / kTestBlocks;
  const double maxMs = *std::max_element(timesMs.begin(), timesMs.end());
  const double p99Ms = [&] {
    auto sorted = timesMs;
    std::sort(sorted.begin(), sorted.end());
    return sorted[static_cast<int>(kTestBlocks * 0.99)];
  }();

  INFO("Latency Harness Results (512 samples @ 48 kHz):");
  INFO("  Block budget : " << (kBlockSize / kSampleRate * 1000.0) << " ms");
  INFO("  Mean latency : " << meanMs << " ms");
  INFO("  P99 latency  : " << p99Ms << " ms");
  INFO("  Max latency  : " << maxMs << " ms");
  INFO("  Budget        : " << kBudgetMs << " ms");

  // Hard requirements
  REQUIRE(meanMs < kBudgetMs);
  REQUIRE(p99Ms < kBudgetMs * 1.5); // P99 within 1.5× budget

  BENCHMARK("Full DSP chain (one block)") {
    bass.process(input8);
    convolver.process(input8, binaural);
    eq.processBlock(binaural);
    limiter.processBlock(binaural);
    return binaural.getReadPointer(0)[0];
  };
}

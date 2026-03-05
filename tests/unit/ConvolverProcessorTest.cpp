// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ConvolverProcessorTest.cpp — Unit tests for ConvolverProcessor (14× JUCE
// Convolution)

#include "../../src/audio/ConvolverProcessor.h"
#include <catch2/catch_all.hpp>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>


using namespace virtuoso;

static HrirSet makeKroneckerHrirSet(double sampleRate = 48000.0) {
  HrirSet hs;
  hs.name = "test-identity";
  for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
    hs.channels[s].left.setSize(1, 512, false, true, false);
    hs.channels[s].right.setSize(1, 512, false, true, false);
    hs.channels[s].left.clear();
    hs.channels[s].right.clear();
    hs.channels[s].left.getWritePointer(0)[0] = 1.0f;
    hs.channels[s].right.getWritePointer(0)[0] = 1.0f;
    hs.channels[s].sampleRate = sampleRate;
  }
  return hs;
}

TEST_CASE("ConvolverProcessor: identity HRIR passes signal to output",
          "[audio][convolver]") {
  ConvolverProcessor convolver;
  juce::dsp::ProcessSpec spec{48000.0, 512, 2};
  convolver.prepare(spec);
  convolver.loadHrir(makeKroneckerHrirSet());

  juce::AudioBuffer<float> input(8, 512);
  juce::AudioBuffer<float> output(2, 512);
  input.clear();
  output.clear();
  input.getWritePointer(0)[0] = 0.5f;

  convolver.process(input, output);

  float maxOut = 0.0f;
  for (int i = 0; i < 512; ++i)
    maxOut = std::max(maxOut, std::abs(output.getReadPointer(0)[i]));
  REQUIRE(maxOut > 0.001f);
}

TEST_CASE("ConvolverProcessor: silent input produces silent output",
          "[audio][convolver]") {
  ConvolverProcessor convolver;
  juce::dsp::ProcessSpec spec{48000.0, 512, 2};
  convolver.prepare(spec);
  convolver.loadHrir(makeKroneckerHrirSet());

  juce::AudioBuffer<float> input(8, 512);
  juce::AudioBuffer<float> output(2, 512);
  input.clear();
  output.clear();

  for (int pass = 0; pass < 5; ++pass)
    convolver.process(input, output);

  for (int ch = 0; ch < 2; ++ch)
    for (int i = 0; i < 512; ++i)
      REQUIRE(output.getReadPointer(ch)[i] ==
              Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("ConvolverProcessor: output is always 2 channels",
          "[audio][convolver]") {
  ConvolverProcessor convolver;
  juce::dsp::ProcessSpec spec{48000.0, 256, 2};
  convolver.prepare(spec);
  convolver.loadHrir(makeKroneckerHrirSet());

  juce::AudioBuffer<float> input(8, 256);
  juce::AudioBuffer<float> output(2, 256);
  input.clear();
  convolver.process(input, output);
  REQUIRE(output.getNumChannels() == 2);
}

TEST_CASE("ConvolverProcessor: hot-swap HRIR does not crash",
          "[audio][convolver]") {
  ConvolverProcessor convolver;
  juce::dsp::ProcessSpec spec{48000.0, 512, 2};
  convolver.prepare(spec);

  juce::AudioBuffer<float> input(8, 512);
  juce::AudioBuffer<float> output(2, 512);
  input.clear();

  REQUIRE_NOTHROW(convolver.loadHrir(makeKroneckerHrirSet(48000.0)));
  REQUIRE_NOTHROW(convolver.process(input, output));
  REQUIRE_NOTHROW(convolver.loadHrir(makeKroneckerHrirSet(44100.0)));
  REQUIRE_NOTHROW(convolver.process(input, output));
}

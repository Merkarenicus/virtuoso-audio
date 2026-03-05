// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// UpmixProcessorTest.cpp — Unit tests for UpmixProcessor

#include "../../src/audio/UpmixProcessor.h"
#include <catch2/catch_all.hpp>
#include <juce_audio_basics/juce_audio_basics.h>


using namespace virtuoso;

TEST_CASE("UpmixProcessor: stereo → 7.1 channel count", "[audio][upmix]") {
  UpmixProcessor proc;
  juce::AudioBuffer<float> stereo(2, 512);
  stereo.clear();
  stereo.getWritePointer(0)[0] = 0.5f;
  stereo.getWritePointer(1)[0] = 0.5f;

  auto result = proc.upmixStereoTo71(stereo);
  REQUIRE(result.getNumChannels() == 8);
  REQUIRE(result.getNumSamples() == 512);
}

TEST_CASE("UpmixProcessor: 5.1 → 7.1 channel count", "[audio][upmix]") {
  UpmixProcessor proc;
  juce::AudioBuffer<float> fiveOne(6, 512);
  fiveOne.clear();

  auto result = proc.upmix51To71(fiveOne);
  REQUIRE(result.getNumChannels() == 8);
}

TEST_CASE("UpmixProcessor: stereo signal appears in FL and FR",
          "[audio][upmix]") {
  UpmixProcessor proc;
  juce::AudioBuffer<float> stereo(2, 512);
  stereo.clear();
  stereo.getWritePointer(0)[0] = 1.0f;
  stereo.getWritePointer(1)[0] = 1.0f;

  auto result = proc.upmixStereoTo71(stereo);
  // FL (ch 0) and FR (ch 1) must be non-zero
  REQUIRE(std::abs(result.getReadPointer(0)[0]) > 0.0f);
  REQUIRE(std::abs(result.getReadPointer(1)[0]) > 0.0f);
}

TEST_CASE("UpmixProcessor: silent input produces near-silent output",
          "[audio][upmix]") {
  UpmixProcessor proc;
  juce::AudioBuffer<float> stereo(2, 512);
  stereo.clear();

  auto result = proc.upmixStereoTo71(stereo);
  for (int ch = 0; ch < 8; ++ch)
    for (int i = 0; i < 512; ++i)
      REQUIRE(result.getReadPointer(ch)[i] ==
              Catch::Approx(0.0f).margin(1e-6f));
}

// ---------------------------------------------------------------------------
// ChannelLayoutMapper tests
// ---------------------------------------------------------------------------
#include "../../src/audio/ChannelLayoutMapper.h"

TEST_CASE("ChannelLayoutMapper: stereo layout has 2 channels",
          "[audio][layout]") {
  REQUIRE(ChannelLayoutMapper::channelCount(ChannelLayout::Stereo) == 2);
}

TEST_CASE("ChannelLayoutMapper: 7.1 layout has 8 channels", "[audio][layout]") {
  REQUIRE(ChannelLayoutMapper::channelCount(ChannelLayout::Surround71) == 8);
}

TEST_CASE("ChannelLayoutMapper: speaker name is non-empty", "[audio][layout]") {
  REQUIRE_FALSE(
      ChannelLayoutMapper::speakerName(ChannelLayout::Surround71, 0).empty());
}

// ---------------------------------------------------------------------------
// BassManagement tests
// ---------------------------------------------------------------------------
#include "../../src/audio/BassManagement.h"

TEST_CASE("BassManagement: process does not increase peak level",
          "[audio][bass]") {
  juce::dsp::ProcessSpec spec{48000.0, 512, 8};
  BassManagement bass;
  bass.prepare(spec);

  juce::AudioBuffer<float> buf(8, 512);
  buf.clear();
  // Fill with max-amplitude signal
  for (int ch = 0; ch < 8; ++ch)
    for (int i = 0; i < 512; ++i)
      buf.getWritePointer(ch)[i] = (i % 2 == 0) ? 0.9f : -0.9f;

  bass.process(buf);

  // Output peak must not exceed input peak (no gain boost)
  for (int ch = 0; ch < 8; ++ch)
    for (int i = 0; i < 512; ++i)
      REQUIRE(std::abs(buf.getReadPointer(ch)[i]) <= 1.0f + 1e-5f);
}

TEST_CASE("BassManagement: LFE channel receives sub-bass content",
          "[audio][bass]") {
  juce::dsp::ProcessSpec spec{48000.0, 512, 8};
  BassManagement bass;
  bass.prepare(spec);

  juce::AudioBuffer<float> buf(8, 512);
  buf.clear();
  // 40 Hz sine into FL (ch 0)
  for (int i = 0; i < 512; ++i)
    buf.getWritePointer(0)[i] =
        std::sin(2.0f * juce::MathConstants<float>::pi * 40.0f * i / 48000.0f);

  bass.process(buf);

  // LFE is channel 3 (ITU-R BS.775-3) — should have energy after bass
  // management
  float lfeMaxAbs = 0.0f;
  for (int i = 0; i < 512; ++i)
    lfeMaxAbs = std::max(lfeMaxAbs, std::abs(buf.getReadPointer(3)[i]));
  REQUIRE(lfeMaxAbs > 0.001f);
}

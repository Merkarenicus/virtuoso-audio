// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// WavHrirParserTest.cpp — Unit tests for WavHrirParser

#include "../../src/hrir/WavHrirParser.h"
#include "../../src/hrir/HrirValidator.h"
#include <catch2/catch_all.hpp>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>


using namespace virtuoso;

// ---------------------------------------------------------------------------
// Helper: synthesise a minimal WAV HRIR in memory and write to a temp file
// ---------------------------------------------------------------------------
static juce::File makeTestWavHrir(int numChannels, int sampleRateHz,
                                  int irLenSamples) {
  juce::File tmpFile = juce::File::createTempFile(".wav");
  juce::AudioFormatManager fmtMgr;
  fmtMgr.registerBasicFormats();
  juce::WavAudioFormat wav;

  auto out = tmpFile.createOutputStream();
  REQUIRE(out->openedOk());
  std::unique_ptr<juce::AudioFormatWriter> writer(
      wav.createWriterFor(out.release(), sampleRateHz, numChannels, 32, {}, 0));
  REQUIRE(writer != nullptr);

  // Fill with a short impulse (Kronecker delta) at sample 16
  juce::AudioBuffer<float> buf(numChannels, irLenSamples);
  buf.clear();
  for (int ch = 0; ch < numChannels; ++ch)
    buf.getWritePointer(ch)[16] = (ch % 2 == 0) ? 0.9f : 0.8f;

  writer->writeFromAudioSampleBuffer(buf, 0, irLenSamples);
  return tmpFile;
}

TEST_CASE("WavHrirParser: HeSuVi 14-channel WAV", "[hrir][wav]") {
  auto tmpFile = makeTestWavHrir(14, 44100, 512);
  WavHrirParser parser;

  SECTION("Parses successfully") {
    auto result = parser.parse(tmpFile);
    REQUIRE(result.has_value());
    REQUIRE(result->channels.size() == HrirSet::kSpeakerCount);
  }

  SECTION("IR length is correct") {
    auto result = parser.parse(tmpFile);
    REQUIRE(result.has_value());
    REQUIRE(result->channels[0].left.getNumSamples() == 512);
  }

  SECTION("Sample rate is captured") {
    auto result = parser.parse(tmpFile);
    REQUIRE(result.has_value());
    REQUIRE(result->channels[0].sampleRate == Catch::Approx(44100.0));
  }

  tmpFile.deleteFile();
}

TEST_CASE("WavHrirParser: standard 8-channel (L/R interleaved) WAV",
          "[hrir][wav]") {
  auto tmpFile = makeTestWavHrir(16, 48000, 256); // 8 speakers × 2 (L+R)
  WavHrirParser parser;

  SECTION("Parses 16-channel as 8 speaker pairs") {
    auto result = parser.parse(tmpFile);
    REQUIRE(result.has_value());
    REQUIRE(result->channels.size() == HrirSet::kSpeakerCount);
  }

  tmpFile.deleteFile();
}

TEST_CASE("WavHrirParser: rejects file with wrong channel count",
          "[hrir][wav]") {
  auto tmpFile = makeTestWavHrir(2, 48000, 512); // Stereo — not valid HRIR
  WavHrirParser parser;

  auto result = parser.parse(tmpFile);
  REQUIRE_FALSE(result.has_value());

  tmpFile.deleteFile();
}

TEST_CASE("HrirValidator: detects NaN in IR data", "[hrir][validator]") {
  juce::AudioBuffer<float> left(1, 64), right(1, 64);
  left.clear();
  right.clear();
  left.getWritePointer(0)[10] = std::numeric_limits<float>::quiet_NaN();

  auto result = HrirValidator::validateChannelPair(left, right, 48000.0);
  REQUIRE_FALSE(result.valid);
  REQUIRE(result.errorMessage.find("NaN") != std::string::npos);
}

TEST_CASE("HrirValidator: detects Inf in IR data", "[hrir][validator]") {
  juce::AudioBuffer<float> left(1, 64), right(1, 64);
  left.clear();
  right.clear();
  right.getWritePointer(0)[32] = std::numeric_limits<float>::infinity();

  auto result = HrirValidator::validateChannelPair(left, right, 48000.0);
  REQUIRE_FALSE(result.valid);
  REQUIRE(result.errorMessage.find("Inf") != std::string::npos);
}

TEST_CASE("HrirValidator: accepts valid impulse response",
          "[hrir][validator]") {
  juce::AudioBuffer<float> left(1, 512), right(1, 512);
  left.clear();
  right.clear();
  left.getWritePointer(0)[16] = 0.9f;
  right.getWritePointer(0)[16] = -0.9f;

  auto result = HrirValidator::validateChannelPair(left, right, 48000.0);
  REQUIRE(result.valid);
}

TEST_CASE("HrirValidator: silent IR raises error (all zeros)",
          "[hrir][validator]") {
  juce::AudioBuffer<float> left(1, 512), right(1, 512);
  left.clear();
  right.clear();

  REQUIRE(HrirValidator::isSilent(left));
  REQUIRE(HrirValidator::isSilent(right));
}

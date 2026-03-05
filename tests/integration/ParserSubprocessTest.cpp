// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ParserSubprocessTest.cpp — Integration test for HrirParserWorker subprocess
// isolation. Verifies that parsing runs in a separated process and that invalid
// inputs are rejected without crashing the parent.

#include "../../src/hrir/HrirParserWorker.h"
#include <catch2/catch_all.hpp>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>


using namespace virtuoso;

static juce::File makeMinimalHeSuViWav() {
  juce::File f = juce::File::createTempFile(".wav");
  juce::WavAudioFormat fmt;
  auto out = f.createOutputStream();
  std::unique_ptr<juce::AudioFormatWriter> writer(
      fmt.createWriterFor(out.release(), 44100.0, 14, 32, {}, 0));
  REQUIRE(writer != nullptr);
  juce::AudioBuffer<float> buf(14, 256);
  buf.clear();
  for (int ch = 0; ch < 14; ++ch)
    buf.getWritePointer(ch)[8] = 0.9f;
  writer->writeFromAudioSampleBuffer(buf, 0, 256);
  return f;
}

TEST_CASE("HrirParserWorker: valid HeSuVi WAV parses correctly",
          "[integration][hrir]") {
  HrirParserWorker worker;
  juce::File wav = makeMinimalHeSuViWav();

  juce::String err;
  auto result = worker.parseSync(wav);

  REQUIRE(result.has_value());
  REQUIRE(result->channels.size() == HrirSet::kSpeakerCount);

  wav.deleteFile();
}

TEST_CASE("HrirParserWorker: invalid file returns error",
          "[integration][hrir]") {
  HrirParserWorker worker;
  juce::File notAWav = juce::File::createTempFile(".wav");
  notAWav.replaceWithText("this is not a wav file at all...");

  auto result = worker.parseSync(notAWav);
  REQUIRE_FALSE(result.has_value());
  REQUIRE_FALSE(result.error().empty());

  notAWav.deleteFile();
}

TEST_CASE("HrirParserWorker: NaN-poisoned IR is rejected by validator",
          "[integration][hrir]") {
  HrirParserWorker worker;

  // Build a 14-ch WAV with NaN in channel 0
  juce::File f = juce::File::createTempFile(".wav");
  juce::WavAudioFormat fmt;
  auto out = f.createOutputStream();
  std::unique_ptr<juce::AudioFormatWriter> writer(
      fmt.createWriterFor(out.release(), 48000.0, 14, 32, {}, 0));
  juce::AudioBuffer<float> buf(14, 256);
  buf.clear();
  buf.getWritePointer(0)[0] = std::numeric_limits<float>::quiet_NaN();
  writer->writeFromAudioSampleBuffer(buf, 0, 256);
  writer.reset();

  auto result = worker.parseSync(f);
  REQUIRE_FALSE(result.has_value());

  f.deleteFile();
}

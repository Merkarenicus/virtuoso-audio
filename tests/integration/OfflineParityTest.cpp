// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// OfflineParityTest.cpp — Integration test: offline render produces valid
// output Both paths use the same ConvolverProcessor and EQ objects.

#include "../../src/audio/BassManagement.h"
#include "../../src/audio/ConvolverProcessor.h"
#include "../../src/audio/EqProcessor.h"
#include "../../src/audio/LimiterProcessor.h"
#include "../../src/export/OfflineRenderer.h"
#include <catch2/catch_all.hpp>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>


using namespace virtuoso;

static juce::File makeStereoSineWav(double hz, double durationSec,
                                    double sampleRate) {
  juce::File f = juce::File::createTempFile(".wav");
  juce::WavAudioFormat fmt;
  auto out = f.createOutputStream();
  std::unique_ptr<juce::AudioFormatWriter> writer(
      fmt.createWriterFor(out.release(), sampleRate, 2, 32, {}, 0));
  REQUIRE(writer != nullptr);

  const int totalSamples = static_cast<int>(durationSec * sampleRate);
  juce::AudioBuffer<float> buf(2, totalSamples);
  for (int i = 0; i < totalSamples; ++i) {
    const float s = std::sin(2.0f * juce::MathConstants<float>::pi * (float)hz *
                             i / (float)sampleRate);
    buf.getWritePointer(0)[i] = s * 0.5f;
    buf.getWritePointer(1)[i] = s * 0.5f;
  }
  writer->writeFromAudioSampleBuffer(buf, 0, totalSamples);
  return f;
}

TEST_CASE("OfflineRenderer: produces valid stereo WAV",
          "[integration][export]") {
  ConvolverProcessor convolver;
  EqProcessor eq;
  BassManagement bass;
  LimiterProcessor limiter;

  OfflineRenderer renderer(convolver, eq, bass, limiter);

  auto inputWav = makeStereoSineWav(440.0, 1.0, 48000.0);
  auto outputWav = juce::File::createTempFile(".out.wav");

  OfflineRenderOptions opts;
  opts.outputSampleRate = 48000.0;
  opts.outputBitDepth = 32;
  opts.blockSize = 512;

  auto result = renderer.render(inputWav, outputWav, opts, nullptr);

  REQUIRE(result.success);
  REQUIRE(result.totalFramesWritten > 0);
  REQUIRE(outputWav.existsAsFile());
  REQUIRE(outputWav.getSize() > 44);

  inputWav.deleteFile();
  outputWav.deleteFile();
}

TEST_CASE("OfflineRenderer: cancel stops render early",
          "[integration][export]") {
  ConvolverProcessor convolver;
  EqProcessor eq;
  BassManagement bass;
  LimiterProcessor limiter;

  OfflineRenderer renderer(convolver, eq, bass, limiter);
  auto inputWav = makeStereoSineWav(1000.0, 30.0, 48000.0);
  auto outputWav = juce::File::createTempFile(".out.wav");

  OfflineRenderOptions opts;
  opts.blockSize = 512;

  int callbackCount = 0;
  auto result = renderer.render(inputWav, outputWav, opts, [&](float) {
    if (callbackCount++ == 0)
      renderer.cancel();
  });

  REQUIRE_FALSE(result.success);
  REQUIRE(result.totalFramesWritten < static_cast<int64_t>(30.0 * 48000.0));

  inputWav.deleteFile();
  outputWav.deleteFile();
}

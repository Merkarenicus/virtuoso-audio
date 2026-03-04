// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// OfflineRenderer.cpp
// Renders an audio file through the full Virtuoso DSP chain to a stereo output
// file. Uses the same DSP objects as real-time playback to guarantee bit-exact
// parity.

#include "OfflineRenderer.h"
#include "../logging/Logger.h"

namespace virtuoso {

OfflineRenderer::OfflineRenderer(ConvolverProcessor &convolver, EqProcessor &eq,
                                 BassManagement &bassManagement,
                                 LimiterProcessor &limiter)
    : m_convolver(convolver), m_eq(eq), m_bassManagement(bassManagement),
      m_limiter(limiter) {}

OfflineRenderResult OfflineRenderer::render(
    const juce::File &inputFile, const juce::File &outputFile,
    const OfflineRenderOptions &opts, ExportProgressCallback progressCallback) {
  VLOG_INFO("OfflineRenderer",
            "Starting render: " + inputFile.getFileName().toStdString() +
                " → " + outputFile.getFileName().toStdString());

  // -------------------------------------------------------------------
  // Open input
  // -------------------------------------------------------------------
  juce::AudioFormatManager fmtMgr;
  fmtMgr.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatReader> reader(
      fmtMgr.createReaderFor(inputFile));
  if (!reader)
    return {false,
            "Cannot open input file: " +
                inputFile.getFullPathName().toStdString(),
            0};

  const double inputSampleRate = reader->sampleRate;
  const int inputChannels = static_cast<int>(reader->numChannels);
  const int64_t totalSamples = reader->lengthInSamples;

  // -------------------------------------------------------------------
  // Set up DSP chain for offline use (reset state to guarantee parity)
  // -------------------------------------------------------------------
  const double engineRate =
      opts.outputSampleRate > 0 ? opts.outputSampleRate : inputSampleRate;
  const int blockSize = opts.blockSize;

  juce::dsp::ProcessSpec stereoSpec{engineRate,
                                    static_cast<uint32_t>(blockSize), 2};
  juce::dsp::ProcessSpec monoSpec{engineRate, static_cast<uint32_t>(blockSize),
                                  1};

  m_convolver.prepare(stereoSpec);
  m_convolver.reset();
  m_bassManagement.prepare(monoSpec);
  m_bassManagement.reset();
  m_eq.prepare(stereoSpec);
  m_eq.reset();
  m_limiter.prepare(stereoSpec);
  m_limiter.reset();

  // Set up input resampler if sample rates differ
  ResamplerProcessor inputResampler;
  bool needsResample = (std::abs(inputSampleRate - engineRate) > 0.5);
  if (needsResample) {
    inputResampler.prepare(inputSampleRate, engineRate, 8);
    VLOG_INFO("OfflineRenderer",
              "Resampling: " + std::to_string(inputSampleRate) + " → " +
                  std::to_string(engineRate));
  }

  // -------------------------------------------------------------------
  // Create output writer
  // -------------------------------------------------------------------
  juce::WavAudioFormat wavFmt;
  outputFile.create();
  auto outStream = outputFile.createOutputStream();
  if (!outStream || !outStream->openedOk())
    return {false,
            "Cannot open output file: " +
                outputFile.getFullPathName().toStdString(),
            0};

  std::unique_ptr<juce::AudioFormatWriter> writer(
      wavFmt.createWriterFor(outStream.release(), engineRate,
                             2, // stereo output
                             opts.outputBitDepth, {}, 0));
  if (!writer)
    return {false, "Cannot create WAV writer", 0};

  // -------------------------------------------------------------------
  // Process blocks
  // -------------------------------------------------------------------
  juce::AudioBuffer<float> inputBuf(inputChannels, blockSize);
  juce::AudioBuffer<float> canonical8ch(8, blockSize);
  juce::AudioBuffer<float> resampledBuf(
      8, blockSize * 2); // Extra capacity for ratio > 1
  juce::AudioBuffer<float> convolverOut(2, blockSize * 2);
  juce::AudioBuffer<float> lfeOut(1, blockSize);

  int64_t samplesRead = 0;
  UpmixProcessor upmixer;

  while (samplesRead < totalSamples) {
    if (m_cancelled.load(std::memory_order_relaxed)) {
      VLOG_INFO("OfflineRenderer",
                "Render cancelled at sample " + std::to_string(samplesRead));
      return {false, "Cancelled", samplesRead};
    }

    const int samplesToRead = static_cast<int>(
        std::min(static_cast<int64_t>(blockSize), totalSamples - samplesRead));

    reader->read(&inputBuf, 0, samplesToRead, samplesRead, true, true);
    samplesRead += samplesToRead;

    // Clear tail if last block is partial
    if (samplesToRead < blockSize)
      for (int ch = 0; ch < inputBuf.getNumChannels(); ++ch)
        juce::FloatVectorOperations::clear(inputBuf.getWritePointer(ch) +
                                               samplesToRead,
                                           blockSize - samplesToRead);

    // Determine format and upmix to 8-channel
    const int inCh = inputBuf.getNumChannels();
    auto format = UpmixProcessor::detectFormat(inCh);
    canonical8ch.clear();
    if (format == UpmixProcessor::SourceFormat::Surround71) {
      for (int ch = 0; ch < 8 && ch < inCh; ++ch)
        juce::FloatVectorOperations::copy(canonical8ch.getWritePointer(ch),
                                          inputBuf.getReadPointer(ch),
                                          blockSize);
    } else {
      upmixer.process(inputBuf, canonical8ch, format, blockSize);
    }

    // Resample if needed
    int processN = blockSize;
    juce::AudioBuffer<float> *chainInput = &canonical8ch;
    if (needsResample) {
      resampledBuf.clear();
      processN = inputResampler.process(canonical8ch, resampledBuf, blockSize);
      chainInput = &resampledBuf;
    }

    // Bass management
    lfeOut.clear();
    m_bassManagement.process(chainInput->getReadPointer(3),
                             lfeOut.getWritePointer(0), processN);

    // Convolve
    convolverOut.clear();
    m_convolver.process(*chainInput, convolverOut);

    // Add LFE to L/R
    juce::FloatVectorOperations::add(convolverOut.getWritePointer(0),
                                     lfeOut.getReadPointer(0), processN);
    juce::FloatVectorOperations::add(convolverOut.getWritePointer(1),
                                     lfeOut.getReadPointer(0), processN);

    // EQ and limiting
    m_eq.process(convolverOut, processN);
    m_limiter.process(convolverOut, processN);

    // Write to output
    writer->writeFromAudioSampleBuffer(convolverOut, 0, processN);

    // Report progress
    if (progressCallback) {
      float pct =
          static_cast<float>(samplesRead) / static_cast<float>(totalSamples);
      progressCallback(pct);
    }
  }

  writer->flush();
  VLOG_INFO("OfflineRenderer",
            "Render complete: " + std::to_string(samplesRead) + " samples");
  return {true, {}, samplesRead};
}

void OfflineRenderer::cancel() noexcept {
  m_cancelled.store(true, std::memory_order_release);
}

} // namespace virtuoso

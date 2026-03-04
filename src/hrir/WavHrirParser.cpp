// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// WavHrirParser.cpp

#include "WavHrirParser.h"
#include "../audio/ConvolverProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace virtuoso {

WavParseResult WavHrirParser::parse(const juce::File &wavFile) const {
  // Pre-parse file size check
  auto sizeCheck = HrirValidator::validateFileSize(wavFile.getSize());
  if (!sizeCheck.valid)
    return std::unexpected(sizeCheck.errorMessage);

  // Use JUCE to load WAV into an AudioBuffer
  juce::AudioFormatManager formatMgr;
  formatMgr.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatReader> reader(
      formatMgr.createReaderFor(wavFile));
  if (!reader)
    return std::unexpected("Cannot open WAV file: " +
                           wavFile.getFullPathName().toStdString());

  // Hard reject: too many channels or too many samples
  if (reader->numChannels >
      static_cast<unsigned int>(HrirValidator::kMaxChannels))
    return std::unexpected("WAV has too many channels: " +
                           std::to_string(reader->numChannels));
  if (reader->lengthInSamples > HrirValidator::kMaxSamplesPerCh)
    return std::unexpected(
        "WAV is too long: " + std::to_string(reader->lengthInSamples) +
        " samples");

  const int numCh = static_cast<int>(reader->numChannels);
  const int numSamples = static_cast<int>(reader->lengthInSamples);
  const double sampleRate = reader->sampleRate;

  juce::AudioBuffer<float> raw(numCh, numSamples);
  reader->read(&raw, 0, numSamples, 0, true, true);

  if (numCh == kHeSuViChannelCount)
    return parseHeSuVi14Ch(raw, sampleRate);

  if (numCh == 8)
    return parse8Ch(raw, sampleRate);

  return std::unexpected(
      "Unsupported WAV channel count: " + std::to_string(numCh) +
      ". Expected 14 (HeSuVi) or 8.");
}

WavParseResult WavHrirParser::parseFromMemory(const void *data,
                                              size_t sizeBytes) const {
  // Pre-parse size check
  auto sizeCheck =
      HrirValidator::validateFileSize(static_cast<int64_t>(sizeBytes));
  if (!sizeCheck.valid)
    return std::unexpected(sizeCheck.errorMessage);

  juce::MemoryBlock block(data, sizeBytes);
  juce::MemoryInputStream memStream(block, false);

  juce::AudioFormatManager formatMgr;
  formatMgr.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatReader> reader(formatMgr.createReaderFor(
      std::make_unique<juce::MemoryInputStream>(block, false)));
  if (!reader)
    return std::unexpected("Cannot parse data as WAV");

  if (reader->numChannels >
      static_cast<unsigned int>(HrirValidator::kMaxChannels))
    return std::unexpected("Too many channels: " +
                           std::to_string(reader->numChannels));
  if (reader->lengthInSamples > HrirValidator::kMaxSamplesPerCh)
    return std::unexpected("WAV too long");

  const int numCh = static_cast<int>(reader->numChannels);
  const int numSamples = static_cast<int>(reader->lengthInSamples);

  juce::AudioBuffer<float> raw(numCh, numSamples);
  reader->read(&raw, 0, numSamples, 0, true, true);

  if (numCh == kHeSuViChannelCount)
    return parseHeSuVi14Ch(raw, reader->sampleRate);
  if (numCh == 8)
    return parse8Ch(raw, reader->sampleRate);

  return std::unexpected("Invalid channel count for HRIR WAV: " +
                         std::to_string(numCh));
}

// ---------------------------------------------------------------------------
// HeSuVi 14-channel layout:
// Ch: 0=FL_L, 1=FL_R, 2=FR_L, 3=FR_R, 4=C_L, 5=C_R,
//     6=SL_L, 7=SL_R, 8=SR_L, 9=SR_R, 10=BL_L, 11=BL_R, 12=BR_L, 13=BR_R
// ---------------------------------------------------------------------------
WavParseResult
WavHrirParser::parseHeSuVi14Ch(const juce::AudioBuffer<float> &raw,
                               double sampleRate) {
  HrirSet hrirSet;
  hrirSet.name = "HeSuVi Import";
  hrirSet.source = "HeSuVi 14-channel WAV";

  static constexpr int kChannelPairs[7][2] = {
      {0, 1},   // FL:  ch0=L, ch1=R
      {2, 3},   // FR:  ch2=L, ch3=R
      {4, 5},   // C:   ch4=L, ch5=R
      {6, 7},   // SL:  ch6=L, ch7=R
      {8, 9},   // SR:  ch8=L, ch9=R
      {10, 11}, // BL:  ch10=L, ch11=R
      {12, 13}, // BR:  ch12=L, ch13=R
  };
  static const char *kLabels[] = {"FL", "FR", "C", "SL", "SR", "BL", "BR"};

  const int irLen = raw.getNumSamples();

  for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
    auto &pair = hrirSet.channels[s];
    pair.left.setSize(1, irLen, false, true, false);
    pair.right.setSize(1, irLen, false, true, false);
    pair.sampleRate = sampleRate;
    pair.label = kLabels[s];

    const float *srcL = raw.getReadPointer(kChannelPairs[s][0]);
    const float *srcR = raw.getReadPointer(kChannelPairs[s][1]);
    juce::FloatVectorOperations::copy(pair.left.getWritePointer(0), srcL,
                                      irLen);
    juce::FloatVectorOperations::copy(pair.right.getWritePointer(0), srcR,
                                      irLen);

    auto valResult =
        HrirValidator::validateChannelPair(pair.left, pair.right, sampleRate);
    if (!valResult.valid)
      return std::unexpected("HeSuVi speaker " + std::string(kLabels[s]) +
                             " validation failed: " + valResult.errorMessage);
  }

  return hrirSet;
}

// ---------------------------------------------------------------------------
// Standard 8-channel 7.1 WAV (FL,FR,C,LFE,BL,BR,SL,SR)
// For HRIR purposes: paired stereo IR files (L=ch0, R=ch1) per speaker.
// This is a simplified layout — real per-speaker HRIR sourcing typically uses
// HeSuVi or SOFA format. 8-ch WAV is treated as 4 stereo pairs:
// pair 0 = FL (ch0/ch1), pair 1 = FR (ch2/ch3), etc.
// ---------------------------------------------------------------------------
WavParseResult WavHrirParser::parse8Ch(const juce::AudioBuffer<float> &raw,
                                       double sampleRate) {
  HrirSet hrirSet;
  hrirSet.name = "8-Channel WAV Import";
  hrirSet.source = "Standard 8-channel WAV";

  // Interpret as 4 stereo pairs for first 4 speakers (FL,FR,C,SL)
  // Remaining speakers (SR,BL,BR) get duplicated mirrors for basic
  // compatibility
  static const char *kLabels[] = {"FL", "FR", "C", "SL", "SR", "BL", "BR"};
  const int irLen = raw.getNumSamples();

  // Simple mapping: take pairs of channels
  for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
    auto &pair = hrirSet.channels[s];
    pair.left.setSize(1, irLen, false, true, false);
    pair.right.setSize(1, irLen, false, true, false);
    pair.sampleRate = sampleRate;
    pair.label = kLabels[s];

    int srcCh = std::min(s * 2, raw.getNumChannels() - 2);
    juce::FloatVectorOperations::copy(pair.left.getWritePointer(0),
                                      raw.getReadPointer(srcCh), irLen);
    juce::FloatVectorOperations::copy(pair.right.getWritePointer(0),
                                      raw.getReadPointer(srcCh + 1), irLen);
  }

  return hrirSet;
}

} // namespace virtuoso

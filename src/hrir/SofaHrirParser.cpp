// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// SofaHrirParser.cpp

#include "SofaHrirParser.h"
#include "../logging/Logger.h"

namespace virtuoso {

std::expected<HrirSet, std::string>
SofaHrirParser::parse(const juce::File &sofaFile) const {
  // File size check
  auto sizeCheck = HrirValidator::validateFileSize(sofaFile.getSize());
  if (!sizeCheck.valid)
    return std::unexpected(sizeCheck.errorMessage);

  // Open SOFA file and resample to 48 kHz (engine default)
  constexpr double kTargetSr = 48000.0;
  auto sofaResult = openSofa(sofaFile, kTargetSr);
  if (!sofaResult.has_value())
    return std::unexpected(sofaResult.error());

  MYSOFA_EASY *sofa = sofaResult->handle;
  HrirSet hrirSet;
  hrirSet.name = sofaFile.getFileNameWithoutExtension().toStdString();
  hrirSet.source = "SOFA: " + sofaFile.getFileName().toStdString();

  for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
    const auto &pos = m_positions[s];
    auto pairResult = extractHrirForPosition(
        sofa, pos.azimuthDeg, pos.elevationDeg, kTargetSr, pos.label);
    if (!pairResult.has_value())
      return std::unexpected("SOFA speaker " + std::string(pos.label) + ": " +
                             pairResult.error());

    hrirSet.channels[s] = std::move(*pairResult);
  }
  return hrirSet;
}

std::expected<SofaHrirParser::SofaHandle, std::string>
SofaHrirParser::openSofa(const juce::File &file, double targetSampleRate) {
  SofaHandle handle;
  handle.handle =
      mysofa_open(file.getFullPathName().toRawUTF8(),
                  static_cast<float>(targetSampleRate), nullptr, &handle.err);
  if (!handle.handle || handle.err != MYSOFA_OK)
    return std::unexpected("mysofa_open failed (error " +
                           std::to_string(handle.err) + ")");
  return handle;
}

std::expected<HrirChannelPair, std::string>
SofaHrirParser::extractHrirForPosition(MYSOFA_EASY *sofa, float azimuthDeg,
                                       float elevationDeg,
                                       double targetSampleRate,
                                       const char *label) {
  // Convert from degrees to Cartesian via mysofa utility
  float coords[3] = {azimuthDeg, elevationDeg, 1.0f};
  mysofa_s2c(coords); // spherical → Cartesian in-place

  const int irLen = sofa->hrtf->N;
  HrirChannelPair pair;
  pair.label = label;
  pair.sampleRate = targetSampleRate;
  pair.left.setSize(1, irLen, false, true, false);
  pair.right.setSize(1, irLen, false, true, false);

  // Nearest-neighbour lookup
  float leftIR[4096] = {}, rightIR[4096] = {};
  float delays[2] = {};
  const int maxIrLen = std::min(irLen, 4096);

  mysofa_getfilter_float(sofa, coords[0], coords[1], coords[2], leftIR, rightIR,
                         &delays[0], &delays[1]);

  std::copy(leftIR, leftIR + maxIrLen, pair.left.getWritePointer(0));
  std::copy(rightIR, rightIR + maxIrLen, pair.right.getWritePointer(0));

  auto valResult = HrirValidator::validateChannelPair(pair.left, pair.right,
                                                      targetSampleRate);
  if (!valResult.valid)
    return std::unexpected(valResult.errorMessage);

  return pair;
}

} // namespace virtuoso

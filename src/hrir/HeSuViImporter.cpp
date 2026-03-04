// SPDX-License-Identifier: MIT
#include "HeSuViImporter.h"
namespace virtuoso {

std::expected<HeSuViImporter::ImportResult, std::string>
HeSuViImporter::importFile(const juce::File &wavFile) {
  WavHrirParser parser;
  auto parseResult = parser.parse(wavFile);
  if (!parseResult.has_value())
    return std::unexpected(parseResult.error());

  ImportResult result;
  result.hrirSet = std::move(*parseResult);

  // IR length gate
  for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
    const int irLen = result.hrirSet.channels[s].left.getNumSamples();
    if (irLen < kMinIrLength || irLen > kMaxIrLength)
      return std::unexpected("IR length " + std::to_string(irLen) +
                             " out of range [" + std::to_string(kMinIrLength) +
                             ", " + std::to_string(kMaxIrLength) + "]");

    // Silent speaker warning (not a hard failure)
    if (HrirValidator::isSilent(result.hrirSet.channels[s].left) &&
        HrirValidator::isSilent(result.hrirSet.channels[s].right)) {
      result.warningMessage += "Speaker " +
                               result.hrirSet.channels[s].label.toStdString() +
                               " is silent. ";
    }
  }
  result.valid = true;
  return result;
}

} // namespace virtuoso

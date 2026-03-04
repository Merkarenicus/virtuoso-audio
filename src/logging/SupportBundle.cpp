// SPDX-License-Identifier: MIT
#include "SupportBundle.h"
#include "../safety/DriverHealthCheck.h"
namespace virtuoso {
bool SupportBundle::create(const juce::File &dest) {
  // Gathers logs + driver status into a .zip (Phase 4: add actual ZIP
  // compression)
  juce::String report =
      DriverHealthCheck::formatReport(DriverHealthCheck::check());
  return dest.withFileExtension(".txt").replaceWithText(report);
}
} // namespace virtuoso

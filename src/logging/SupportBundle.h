// SPDX-License-Identifier: MIT
#pragma once
#include <juce_core/juce_core.h>
namespace virtuoso {

class SupportBundle {
public:
  // Creates a ZIP at destinationFile with logs, driver status, and system info
  static bool create(const juce::File &destinationFile);
};

} // namespace virtuoso

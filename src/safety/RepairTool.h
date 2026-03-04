// SPDX-License-Identifier: MIT
#pragma once
#include <juce_core/juce_core.h>
namespace virtuoso {

// Repairs common installation issues: re-registers driver, restores default
// audio device
class RepairTool {
public:
  static bool repairWindowsDriver(); // Re-runs driver .inf installer
  static bool repairMacOSDriver();   // Reloads AudioServerPlugIn
  static bool repairLinuxSink();     // Re-adds PipeWire null-sink
  static bool repair();              // Platform dispatch
};

} // namespace virtuoso

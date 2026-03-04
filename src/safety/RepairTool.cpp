// SPDX-License-Identifier: MIT
#include "RepairTool.h"
namespace virtuoso {
bool RepairTool::repair() {
#if defined(_WIN32)
  return repairWindowsDriver();
#elif defined(__APPLE__)
  return repairMacOSDriver();
#else
  return repairLinuxSink();
#endif
}
bool RepairTool::repairWindowsDriver() { return false; /* Phase 4 */ }
bool RepairTool::repairMacOSDriver() { return false; /* Phase 4 */ }
bool RepairTool::repairLinuxSink() { return false; /* Phase 4 */ }
} // namespace virtuoso

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// DeviceWatchdog.cpp

#include "DeviceWatchdog.h"
#include "../logging/Logger.h"

namespace virtuoso {

void DeviceWatchdog::start(WatchdogCallback callback,
                           const juce::String &expectedOutputDeviceId,
                           const juce::String &expectedVirtualDeviceName) {
  m_callback = std::move(callback);
  m_expectedOutputId = expectedOutputDeviceId;
  m_virtualDeviceName = expectedVirtualDeviceName;

  // Do an initial check on start
  timerCallback();
  startTimer(kPollIntervalMs);

  VLOG_INFO("DeviceWatchdog", "Watchdog started. Monitoring: " +
                                  m_virtualDeviceName.toStdString());
}

void DeviceWatchdog::stop() {
  stopTimer();
  VLOG_INFO("DeviceWatchdog", "Watchdog stopped");
}

void DeviceWatchdog::timerCallback() {
  // Check if the Virtuoso virtual device is present in the OS device list
  bool wasPresent = m_virtualDriverPresent.load(std::memory_order_relaxed);
  bool isPresent = checkDevicePresent(m_virtualDeviceName);
  m_virtualDriverPresent.store(isPresent, std::memory_order_relaxed);

  if (wasPresent && !isPresent) {
    VLOG_WARNING("DeviceWatchdog", "Virtual driver disappeared: " +
                                       m_virtualDeviceName.toStdString());
    if (m_callback)
      m_callback(WatchdogEvent::DriverMissing, m_virtualDeviceName);
  } else if (!wasPresent && isPresent) {
    VLOG_INFO("DeviceWatchdog",
              "Virtual driver restored: " + m_virtualDeviceName.toStdString());
    if (m_callback)
      m_callback(WatchdogEvent::DriverRestored, m_virtualDeviceName);
  }

  // Check if expected output device is still present
  if (m_expectedOutputId.isNotEmpty()) {
    bool outputPresent = checkDevicePresent(m_expectedOutputId);
    if (!outputPresent) {
      VLOG_WARNING("DeviceWatchdog",
                   "Output device lost: " + m_expectedOutputId.toStdString());
      if (m_callback)
        m_callback(WatchdogEvent::DeviceLost, m_expectedOutputId);
    }
  }
}

void DeviceWatchdog::changeListenerCallback(juce::ChangeBroadcaster *) {
  // Device manager notified us — schedule a check on next timer tick
  // (or check immediately if timer isn't running)
  if (!isTimerRunning())
    timerCallback();
}

bool DeviceWatchdog::checkDevicePresent(
    const juce::String &deviceName) const noexcept {
  if (deviceName.isEmpty())
    return true;

  // Use a temporary device type list to enumerate current devices
  // (this is done on the UI thread, never on the audio thread)
  juce::AudioDeviceManager tempMgr;
  for (auto *deviceType : tempMgr.getAvailableDeviceTypes()) {
    deviceType->scanForDevices();
    const auto outputNames = deviceType->getDeviceNames(false);
    if (outputNames.contains(deviceName))
      return true;
  }
  return false;
}

} // namespace virtuoso

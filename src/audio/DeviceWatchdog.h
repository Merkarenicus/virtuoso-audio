// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// DeviceWatchdog.h
// Monitors audio device health: hot-plug/unplug, sample rate changes,
// virtual device driver disappearance. Triggers graceful fallback to
// passthrough and can post engine commands to recover automatically.

#pragma once
#include <atomic>
#include <functional>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>


namespace virtuoso {

enum class WatchdogEvent {
  DeviceLost,        // Virtual or output device disappeared
  DeviceRestored,    // Previously lost device came back
  SampleRateChanged, // Device changed sample rate under us
  DriverMissing,     // Virtuoso virtual driver not found in device list
  DriverRestored     // Virtuoso virtual driver appeared in device list
};

using WatchdogCallback =
    std::function<void(WatchdogEvent, const juce::String &deviceId)>;

class DeviceWatchdog : public juce::Timer, public juce::ChangeListener {
public:
  static constexpr int kPollIntervalMs = 2000; // Check every 2 seconds
  static constexpr int kRecoveryDelayMs =
      500; // Delay before auto-recovery attempt

  DeviceWatchdog() = default;
  ~DeviceWatchdog() override { stop(); }

  // Start watchdog. engine must outlive this object.
  void
  start(WatchdogCallback callback, const juce::String &expectedOutputDeviceId,
        const juce::String &expectedVirtualDeviceName = "Virtuoso Virtual 7.1");
  void stop();

  // Notify when audio engine sample rate is confirmed
  void setCurrentSampleRate(double rate) noexcept {
    m_lastKnownSampleRate.store(rate, std::memory_order_relaxed);
  }

  // Returns true if virtual driver is currently visible in OS device list
  bool isVirtualDriverPresent() const noexcept {
    return m_virtualDriverPresent.load(std::memory_order_relaxed);
  }

  // juce::Timer
  void timerCallback() override;

  // juce::ChangeListener (listens to AudioDeviceManager changes)
  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

private:
  WatchdogCallback m_callback;
  juce::String m_expectedOutputId;
  juce::String m_virtualDeviceName;

  std::atomic<bool> m_virtualDriverPresent{false};
  std::atomic<double> m_lastKnownSampleRate{48000.0};

  juce::AudioDeviceManager
      m_monitorDevMgr; // Separate instance for enumeration (no audio)

  bool checkDevicePresent(const juce::String &deviceName) const noexcept;
};

} // namespace virtuoso

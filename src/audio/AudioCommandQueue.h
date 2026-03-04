// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// AudioCommandQueue.h
// Lock-free command queue for UI thread → Audio thread communication.
// Based on a single-producer, single-consumer (SPSC) ring buffer.
// NEVER use mutexes or blocking waits on the audio thread side.
//
// Commands are small value types. Large data (HRIRs) are passed via
// shared_ptr inside the command so no per-command heap allocation occurs
// on the audio thread.

#pragma once
#include "ConvolverProcessor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <variant>


namespace virtuoso {

// ---------------------------------------------------------------------------
// Command types
// ---------------------------------------------------------------------------
struct CmdSetEnabled {
  bool enabled;
};
struct CmdSetVolume {
  float linearGain;
}; // Master volume
struct CmdLoadHrir {
  std::shared_ptr<const HrirSet> hrirSet;
};
struct CmdSetEqBandGain {
  int bandIndex;
  float gainDb;
};
struct CmdSetEqBandFreq {
  int bandIndex;
  float freqHz;
};
struct CmdSetEqBandQ {
  int bandIndex;
  float q;
};
struct CmdSetEqEnabled {
  bool enabled;
};
struct CmdSetBufferSize {
  int framesPerBlock;
};
struct CmdSetOutputDevice {
  juce::String deviceId;
};

using AudioCommand =
    std::variant<CmdSetEnabled, CmdSetVolume, CmdLoadHrir, CmdSetEqBandGain,
                 CmdSetEqBandFreq, CmdSetEqBandQ, CmdSetEqEnabled,
                 CmdSetBufferSize, CmdSetOutputDevice>;

// ---------------------------------------------------------------------------
// AudioCommandQueue
// ---------------------------------------------------------------------------
class AudioCommandQueue {
public:
  static constexpr int kCapacity = 256;

  AudioCommandQueue() : m_abstractFifo(kCapacity), m_buffer(kCapacity) {}

  // UI thread: post a command. Returns false if queue is full (backpressure).
  bool postCommand(AudioCommand cmd) noexcept;

  // Audio thread: process all pending commands, calling handler for each.
  // Do not call blocking code inside handler.
  template <typename Handler> void drainCommands(Handler &&handler) noexcept {
    int start, count;
    m_abstractFifo.prepareToRead(m_abstractFifo.getNumReady(), start, count);
    if (count == 0)
      return;
    for (int i = 0; i < count; ++i) {
      handler(m_buffer[static_cast<size_t>((start + i) % kCapacity)]);
    }
    m_abstractFifo.finishedRead(count);
  }

  int pendingCount() const noexcept { return m_abstractFifo.getNumReady(); }

private:
  juce::AbstractFifo m_abstractFifo;
  std::vector<AudioCommand> m_buffer;
};

} // namespace virtuoso

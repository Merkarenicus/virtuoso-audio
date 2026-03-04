// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// AudioCommandQueue.cpp

#include "AudioCommandQueue.h"

namespace virtuoso {

bool AudioCommandQueue::postCommand(AudioCommand cmd) noexcept {
  return m_queue.push(std::move(cmd));
}

void AudioCommandQueue::drainCommands(
    std::function<void(const AudioCommand &)> handler) noexcept {
  AudioCommand cmd;
  while (m_queue.pop(cmd)) {
    if (handler)
      handler(cmd);
  }
}

} // namespace virtuoso

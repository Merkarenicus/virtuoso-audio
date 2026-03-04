// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// TransactionJournal.h
// Atomic state journal for installer/uninstaller operations.
// Ensures "never break audio" guarantee by recording pre-state before any
// OS audio configuration change and enabling rollback on failure/crash.
//
// State machine:
//   IDLE → SNAPSHOT → APPLYING → VERIFYING → COMMITTED
//                                     ↓ (failure or unclean shutdown)
//                                ROLLING_BACK → ROLLED_BACK
//
// On app startup: check for incomplete transactions and auto-rollback.

#pragma once
#include <functional>
#include <juce_core/juce_core.h>
#include <optional>
#include <string>


namespace virtuoso {

enum class JournalState {
  Idle,
  Snapshot,
  Applying,
  Verifying,
  Committed,
  RollingBack,
  RolledBack,
  Failed
};

struct AudioSystemSnapshot {
  // Windows
  std::string defaultPlaybackDeviceId;
  std::string defaultCaptureDeviceId;
  bool virtuosoDriverInstalled{false};
  std::string virtuosoDriverVersion;

  // macOS
  std::string coreAudioDefaultOutputUID;
  bool audioServerPlugInLoaded{false};

  // Linux
  std::string pipeWireDefaultSinkName;
  std::string pulseAudioDefaultSink;

  // Serialise / deserialise to/from JSON
  juce::var toVar() const;
  static std::optional<AudioSystemSnapshot> fromVar(const juce::var &v);
};

class TransactionJournal {
public:
  explicit TransactionJournal(const juce::File &journalFile);
  ~TransactionJournal() = default;

  // -----------------------------------------------------------------------
  // Transaction lifecycle (call in order)
  // -----------------------------------------------------------------------

  // 1. Capture pre-state. Returns false if journalFile cannot be written.
  bool beginTransaction(const std::string &actionDescription);

  // 2. Record the current audio system state as the "before" snapshot.
  bool snapshotPreState(const AudioSystemSnapshot &snapshot);

  // 3. Mark that we are in the process of applying the change.
  bool markApplying();

  // 4. Mark successful completion.
  bool commit();

  // 5. Roll back to pre-state snapshot. Invokes provided rollback function.
  bool rollback(std::function<bool(const AudioSystemSnapshot &)> restoreFunc);

  // -----------------------------------------------------------------------
  // Startup recovery
  // -----------------------------------------------------------------------

  // Returns true if there is an incomplete transaction (crash during install)
  bool hasIncompleteTransaction() const noexcept {
    return m_state != JournalState::Idle &&
           m_state != JournalState::Committed &&
           m_state != JournalState::RolledBack;
  }

  // Attempt to recover: rollback any incomplete transaction.
  bool
  recoverIfNeeded(std::function<bool(const AudioSystemSnapshot &)> restoreFunc);

  // Read the stored pre-state snapshot (if any)
  std::optional<AudioSystemSnapshot> getPreStateSnapshot() const noexcept {
    return m_preState;
  }

  // Current state
  JournalState state() const noexcept { return m_state; }
  std::string lastError() const { return m_lastError; }

private:
  juce::File m_journalFile;
  JournalState m_state{JournalState::Idle};
  std::optional<AudioSystemSnapshot> m_preState;
  std::string m_lastError;
  std::string m_actionDescription;

  bool persistState(); // Write state to journal file atomically
  bool loadState();    // Read state from journal file on startup
};

} // namespace virtuoso

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// TransactionJournal.cpp

#include "TransactionJournal.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace virtuoso {

// ---------------------------------------------------------------------------
// AudioSystemSnapshot serialisation
// ---------------------------------------------------------------------------

juce::var AudioSystemSnapshot::toVar() const {
  auto *obj = new juce::DynamicObject();
  obj->setProperty("defaultPlaybackDeviceId",
                   juce::String(defaultPlaybackDeviceId));
  obj->setProperty("defaultCaptureDeviceId",
                   juce::String(defaultCaptureDeviceId));
  obj->setProperty("virtuosoDriverInstalled", virtuosoDriverInstalled);
  obj->setProperty("virtuosoDriverVersion",
                   juce::String(virtuosoDriverVersion));
  obj->setProperty("coreAudioDefaultOutputUID",
                   juce::String(coreAudioDefaultOutputUID));
  obj->setProperty("audioServerPlugInLoaded", audioServerPlugInLoaded);
  obj->setProperty("pipeWireDefaultSinkName",
                   juce::String(pipeWireDefaultSinkName));
  obj->setProperty("pulseAudioDefaultSink",
                   juce::String(pulseAudioDefaultSink));
  return juce::var(obj);
}

std::optional<AudioSystemSnapshot>
AudioSystemSnapshot::fromVar(const juce::var &v) {
  if (!v.isObject())
    return std::nullopt;
  auto *obj = v.getDynamicObject();
  if (!obj)
    return std::nullopt;

  AudioSystemSnapshot s;
  s.defaultPlaybackDeviceId =
      obj->getProperty("defaultPlaybackDeviceId").toString().toStdString();
  s.defaultCaptureDeviceId =
      obj->getProperty("defaultCaptureDeviceId").toString().toStdString();
  s.virtuosoDriverInstalled =
      static_cast<bool>(obj->getProperty("virtuosoDriverInstalled"));
  s.virtuosoDriverVersion =
      obj->getProperty("virtuosoDriverVersion").toString().toStdString();
  s.coreAudioDefaultOutputUID =
      obj->getProperty("coreAudioDefaultOutputUID").toString().toStdString();
  s.audioServerPlugInLoaded =
      static_cast<bool>(obj->getProperty("audioServerPlugInLoaded"));
  s.pipeWireDefaultSinkName =
      obj->getProperty("pipeWireDefaultSinkName").toString().toStdString();
  s.pulseAudioDefaultSink =
      obj->getProperty("pulseAudioDefaultSink").toString().toStdString();
  return s;
}

// ---------------------------------------------------------------------------
// TransactionJournal
// ---------------------------------------------------------------------------

TransactionJournal::TransactionJournal(const juce::File &journalFile)
    : m_journalFile(journalFile) {
  loadState(); // Recover any incomplete transaction from previous session
}

bool TransactionJournal::beginTransaction(
    const std::string &actionDescription) {
  if (m_state != JournalState::Idle && m_state != JournalState::Committed &&
      m_state != JournalState::RolledBack) {
    m_lastError = "Cannot begin transaction while one is in progress (state=" +
                  std::to_string(static_cast<int>(m_state)) + ")";
    return false;
  }
  m_actionDescription = actionDescription;
  m_preState.reset();
  m_state = JournalState::Snapshot;
  return persistState();
}

bool TransactionJournal::snapshotPreState(const AudioSystemSnapshot &snapshot) {
  if (m_state != JournalState::Snapshot) {
    m_lastError = "snapshotPreState called out of sequence";
    return false;
  }
  m_preState = snapshot;
  return persistState();
}

bool TransactionJournal::markApplying() {
  if (m_state != JournalState::Snapshot) {
    m_lastError = "markApplying called out of sequence";
    return false;
  }
  m_state = JournalState::Applying;
  return persistState();
}

bool TransactionJournal::commit() {
  m_state = JournalState::Committed;
  bool ok = persistState();
  // Committed — safe to delete journal file
  if (ok)
    m_journalFile.deleteFile();
  return ok;
}

bool TransactionJournal::rollback(
    std::function<bool(const AudioSystemSnapshot &)> restoreFunc) {
  m_state = JournalState::RollingBack;
  persistState();

  if (!m_preState.has_value()) {
    m_lastError = "rollback called but no pre-state snapshot exists";
    m_state = JournalState::Failed;
    persistState();
    return false;
  }

  bool restored = restoreFunc(*m_preState);
  m_state = restored ? JournalState::RolledBack : JournalState::Failed;
  persistState();
  if (m_state == JournalState::RolledBack)
    m_journalFile.deleteFile();
  return restored;
}

bool TransactionJournal::recoverIfNeeded(
    std::function<bool(const AudioSystemSnapshot &)> restoreFunc) {
  if (!hasIncompleteTransaction())
    return true;
  return rollback(std::move(restoreFunc));
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

bool TransactionJournal::persistState() {
  // Write atomically: write to .tmp then rename
  juce::File tmpFile = m_journalFile.withFileExtension(".tmp");
  auto out = tmpFile.createOutputStream();
  if (!out || !out->openedOk()) {
    m_lastError = "Cannot write journal to " +
                  m_journalFile.getFullPathName().toStdString();
    return false;
  }

  auto *obj = new juce::DynamicObject();
  obj->setProperty("state", static_cast<int>(m_state));
  obj->setProperty("action", juce::String(m_actionDescription));
  if (m_preState.has_value())
    obj->setProperty("preState", m_preState->toVar());

  juce::String json = juce::JSON::toString(juce::var(obj), false);
  out->writeText(json, false, false, nullptr);
  out->flush();
  out = nullptr; // Close before rename

  tmpFile.moveFileTo(m_journalFile);
  return true;
}

bool TransactionJournal::loadState() {
  if (!m_journalFile.existsAsFile())
    return true; // No journal = clean state, nothing to do

  juce::var parsed = juce::JSON::parse(m_journalFile.loadFileAsString());
  if (!parsed.isObject())
    return false;

  auto *obj = parsed.getDynamicObject();
  if (!obj)
    return false;

  m_state =
      static_cast<JournalState>(static_cast<int>(obj->getProperty("state")));
  m_actionDescription = obj->getProperty("action").toString().toStdString();

  const juce::var &pre = obj->getProperty("preState");
  if (pre.isObject())
    m_preState = AudioSystemSnapshot::fromVar(pre);

  return true;
}

} // namespace virtuoso

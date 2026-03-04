// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// Logger.cpp

#include "Logger.h"
#include <juce_core/juce_core.h>

namespace virtuoso {

Logger &Logger::instance() {
  static Logger s_instance;
  return s_instance;
}

void Logger::initialise(const juce::File &logDirectory,
                        int64_t maxFileSizeBytes, int maxFileCount) {
  juce::ScopedLock lock(m_mutex);
  if (m_initialised)
    return;

  m_logDir = logDirectory;
  m_maxFileSizeBytes = maxFileSizeBytes;
  m_maxFileCount = maxFileCount;

  logDirectory.createDirectory();

  // Open current log file
  m_currentFile = logDirectory.getChildFile(
      "virtuoso-" + juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S") +
      ".log");
  m_stream = m_currentFile.createOutputStream().release();

  if (m_stream && m_stream->openedOk()) {
    m_initialised = true;
    // Write a JSON-lines header entry
    Entry header;
    header.level = LogLevel::Info;
    header.timestamp = juce::Time::getCurrentTime();
    header.subsystem = "Logger";
    header.message = "Log session started; version=" VIRTUOSO_VERSION_STRING;
    writeEntry(header);
  }
}

void Logger::log(LogLevel level, std::string_view subsystem,
                 std::string_view message, const char *file,
                 int line) noexcept {
  if (static_cast<int>(level) < m_minLevel.load(std::memory_order_relaxed))
    return;

  Entry e;
  e.level = level;
  e.timestamp = juce::Time::getCurrentTime();
  e.subsystem = juce::String(subsystem.data(), subsystem.size());
  e.message = juce::String(message.data(), message.size());
  if (file) {
    // Strip full path — keep only filename
    juce::String fullPath(file);
    e.file = fullPath.fromLastOccurrenceOf("/", false, false)
                 .fromLastOccurrenceOf("\\", false, false);
  }
  e.line = line;

  juce::ScopedLock lock(m_mutex);
  if (!m_initialised)
    return;
  rotateIfNeeded();
  writeEntry(e);
}

void Logger::flush() noexcept {
  juce::ScopedLock lock(m_mutex);
  if (m_stream)
    m_stream->flush();
}

void Logger::shutdown() noexcept {
  juce::ScopedLock lock(m_mutex);
  if (m_stream) {
    Entry footer;
    footer.level = LogLevel::Info;
    footer.timestamp = juce::Time::getCurrentTime();
    footer.subsystem = "Logger";
    footer.message = "Log session ended.";
    writeEntry(footer);
    m_stream->flush();
    delete m_stream;
    m_stream = nullptr;
  }
  m_initialised = false;
}

juce::File Logger::currentLogFile() const noexcept {
  juce::ScopedLock lock(m_mutex);
  return m_currentFile;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void Logger::rotateIfNeeded() {
  if (!m_stream)
    return;
  if (m_currentFile.getSize() < m_maxFileSizeBytes)
    return;

  // Close current
  m_stream->flush();
  delete m_stream;
  m_stream = nullptr;

  // Open a new file
  m_currentFile = m_logDir.getChildFile(
      "virtuoso-" + juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S") +
      ".log");
  m_stream = m_currentFile.createOutputStream().release();

  // Purge oldest files if we exceed maxFileCount
  juce::Array<juce::File> logFiles;
  m_logDir.findChildFiles(logFiles, juce::File::findFiles, false,
                          "virtuoso-*.log");
  logFiles.sort();
  while (logFiles.size() > m_maxFileCount) {
    logFiles.getFirst().deleteFile();
    logFiles.remove(0);
  }
}

void Logger::writeEntry(const Entry &e) {
  if (!m_stream || !m_stream->openedOk())
    return;

  // JSON-lines format:
  // {"t":"2026-03-04T19:34:00.000Z","lvl":"INFO","sys":"AudioEngine","msg":"...","file":"foo.cpp","line":42}
  static const char *kLevels[] = {"DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"};
  const char *lvlStr = kLevels[std::min(static_cast<int>(e.level), 4)];

  juce::String line =
      "{\"t\":\"" + e.timestamp.toISO8601(true) + "\",\"lvl\":\"" + lvlStr +
      "\",\"sys\":\"" + e.subsystem.replace("\"", "'") + "\",\"msg\":\"" +
      e.message.replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
  if (e.file.isNotEmpty())
    line += ",\"file\":\"" + e.file + "\",\"line\":" + juce::String(e.line);
  line += "}\n";

  m_stream->writeText(line, false, false, nullptr);
}

juce::String Logger::formatEntry(const Entry &e) const {
  static const char *kLevels[] = {"DBG", "INF", "WRN", "ERR", "CRT"};
  return juce::String("[") + kLevels[static_cast<int>(e.level)] + "] " +
         e.subsystem + ": " + e.message;
}

} // namespace virtuoso

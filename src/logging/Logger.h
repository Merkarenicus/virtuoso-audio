// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// Logger.h
// Rotating structured log system. Thread-safe. Never throws.
// Logs written to platform-appropriate location:
//   Windows: %LOCALAPPDATA%\VirtuosoAudio\logs\
//   macOS:   ~/Library/Logs/VirtuosoAudio/
//   Linux:   ~/.local/share/virtuoso-audio/logs/
//
// Format: JSON-lines, one entry per line for easy machine parsing.
// Rotation: 10 files × 1 MB each (configurable).

#pragma once
#include <atomic>
#include <juce_core/juce_core.h>
#include <string_view>


namespace virtuoso {

enum class LogLevel { Debug = 0, Info, Warning, Error, Critical };

class Logger {
public:
  // Singleton — call initialise() once at startup before any log calls.
  static Logger &instance();

  // Initialise logging. Call from main thread before any other code.
  // maxFileSizeBytes: rotate when log file exceeds this size (default 1 MB)
  // maxFileCount: number of rotated files to keep (default 10)
  void initialise(const juce::File &logDirectory,
                  int64_t maxFileSizeBytes = 1024 * 1024,
                  int maxFileCount = 10);

  // Log a message. Thread-safe. Non-blocking (writes to ring buffer,
  // flushed by background thread).
  void log(LogLevel level,
           std::string_view subsystem, // e.g. "AudioEngine", "HrirParser"
           std::string_view message,
           const char *file = nullptr, // __FILE__
           int line = 0) noexcept;

  // Convenience macros provide __FILE__ / __LINE__ automatically (defined
  // below)

  // Flush all pending log entries to disk. Call on graceful shutdown.
  void flush() noexcept;

  // Close the log file. Call at process exit.
  void shutdown() noexcept;

  // Current minimum log level (entries below this are discarded)
  void setMinLevel(LogLevel level) noexcept {
    m_minLevel.store(static_cast<int>(level), std::memory_order_relaxed);
  }
  LogLevel minLevel() const noexcept {
    return static_cast<LogLevel>(m_minLevel.load(std::memory_order_relaxed));
  }

  // Returns the path to the most recent log file
  juce::File currentLogFile() const noexcept;

private:
  Logger() = default;
  ~Logger() { shutdown(); }

  struct Entry {
    LogLevel level;
    juce::Time timestamp;
    juce::String subsystem;
    juce::String message;
    juce::String file;
    int line;
  };

  juce::File m_logDir;
  juce::File m_currentFile;
  juce::FileOutputStream *m_stream{nullptr};
  juce::CriticalSection m_mutex;
  std::atomic<int> m_minLevel{static_cast<int>(LogLevel::Debug)};
  int64_t m_maxFileSizeBytes{1024 * 1024};
  int m_maxFileCount{10};
  bool m_initialised{false};

  void rotateIfNeeded();
  void writeEntry(const Entry &e);
  juce::String formatEntry(const Entry &e) const;
};

// ---------------------------------------------------------------------------
// Convenience macros
// ---------------------------------------------------------------------------
#define VLOG_DEBUG(subsys, msg)                                                \
  ::virtuoso::Logger::instance().log(::virtuoso::LogLevel::Debug, (subsys),    \
                                     (msg), __FILE__, __LINE__)
#define VLOG_INFO(subsys, msg)                                                 \
  ::virtuoso::Logger::instance().log(::virtuoso::LogLevel::Info, (subsys),     \
                                     (msg), __FILE__, __LINE__)
#define VLOG_WARNING(subsys, msg)                                              \
  ::virtuoso::Logger::instance().log(::virtuoso::LogLevel::Warning, (subsys),  \
                                     (msg), __FILE__, __LINE__)
#define VLOG_ERROR(subsys, msg)                                                \
  ::virtuoso::Logger::instance().log(::virtuoso::LogLevel::Error, (subsys),    \
                                     (msg), __FILE__, __LINE__)
#define VLOG_CRITICAL(subsys, msg)                                             \
  ::virtuoso::Logger::instance().log(::virtuoso::LogLevel::Critical, (subsys), \
                                     (msg), __FILE__, __LINE__)

} // namespace virtuoso

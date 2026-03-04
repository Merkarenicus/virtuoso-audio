// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HrirParserWorker.h
// Subprocess isolation for HRIR parsing.
// ALL WAV/SOFA parsing runs in a separate process with strict resource limits.
// The parent process communicates via a simple JSON IPC protocol over stdio.
//
// Security model (from ADR and all swarm reviews):
//   - Parser subprocess inherits NO audio device handles
//   - Parser subprocess runs with reduced privileges (see platform notes below)
//   - Parent sends: {cmd:"parse", path:"...", format:"wav|sofa",
//   maxFileSizeBytes:N}
//   - Worker sends: {ok:true, type:"hrir_set", data:{...}} OR {ok:false,
//   error:"..."}
//   - Hard kill after 30 seconds (hung parser = malformed file attack)
//
// Platform privilege reduction:
//   Windows: Job object with JOBOBJECT_BASIC_UI_RESTRICTIONS +
//   CreateRestrictedToken macOS:   sandbox_init(kSBXProfileNoNetwork) Linux:
//   seccomp-bpf filter (read, write, mmap, exit only)

#pragma once
#include "WavHrirParser.h"
#include <atomic>
#include <functional>
#include <future>
#include <juce_core/juce_core.h>
#include <optional>


namespace virtuoso {

// Result returned from the subprocess after parsing completes
using HrirParseResult = std::expected<HrirSet, std::string>;

// IPC message format (JSON-lines over stdin/stdout of subprocess)
struct HrirParseRequest {
  juce::String filePath;
  juce::String format; // "wav" or "sofa"
  int64_t maxFileBytes{50 * 1024 * 1024};
};

class HrirParserWorker {
public:
  static constexpr int kTimeoutMs = 30000; // 30 seconds max per parse

  HrirParserWorker() = default;
  ~HrirParserWorker() { shutdown(); }

  // Not copyable
  HrirParserWorker(const HrirParserWorker &) = delete;
  HrirParserWorker &operator=(const HrirParserWorker &) = delete;

  // Asynchronously parse a file in a subprocess.
  // The returned future resolves when parsing completes, times out, or crashes.
  // Calls resultCallback on the UI thread (via
  // juce::MessageManager::callAsync).
  std::future<HrirParseResult>
  parseAsync(const juce::File &file,
             std::function<void(HrirParseResult)> resultCallback = nullptr);

  // Synchronous parse (blocks calling thread). Intended for offline rendering
  // only.
  HrirParseResult parseSync(const juce::File &file);

  // Kill any running subprocess immediately
  void shutdown() noexcept;

  bool isRunning() const noexcept {
    return m_running.load(std::memory_order_relaxed);
  }

private:
  std::atomic<bool> m_running{false};
  std::unique_ptr<juce::ChildProcess> m_childProcess;
  juce::CriticalSection m_lock;

  // Spawn the subprocess and send a parse request
  // Returns the subprocess stdout line containing the result JSON
  std::optional<juce::String> spawnAndParse(const HrirParseRequest &req);

  // Deserialise HrirSet from JSON string sent by subprocess
  static HrirParseResult deserialiseResult(const juce::String &jsonLine);

  // Serialise HrirSet to JSON for IPC (used by the subprocess side)
  static juce::String serialiseHrirSet(const HrirSet &hrirSet);

  // -----------------------------------------------------------------------
  // Subprocess entry point — called when this binary is invoked as a worker:
  //   virtuoso-audio --hrir-parser-worker
  // Reads one JSON line from stdin, parses, writes one JSON result to stdout.
  // -----------------------------------------------------------------------
public:
  static int workerMain(); // Returns 0 on success, 1 on error
};

} // namespace virtuoso

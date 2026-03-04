// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// HrirParserWorker.cpp

#include "HrirParserWorker.h"
#include "WavHrirParser.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace virtuoso {

// ---------------------------------------------------------------------------
// Subprocess entry point
// Called when binary detects --hrir-parser-worker argument.
// Protocol: reads one JSON line from stdin, parses, writes result to stdout.
// ---------------------------------------------------------------------------
int HrirParserWorker::workerMain() {
  // Read one request line from stdin
  std::string requestLine;
  if (!std::getline(std::cin, requestLine) || requestLine.empty()) {
    std::cout << R"({"ok":false,"error":"Empty request"})" << "\n";
    return 1;
  }

  try {
    auto req = json::parse(requestLine);
    const std::string filePath = req.value("path", "");
    const std::string format = req.value("format", "wav");
    const int64_t maxBytes =
        req.value("maxFileBytes", int64_t(50 * 1024 * 1024));

    juce::File file(juce::String(filePath));
    if (!file.existsAsFile()) {
      std::cout << json{{"ok", false}, {"error", "File not found: " + filePath}}
                       .dump()
                << "\n";
      return 1;
    }

    // Pre-parse size check
    auto sizeCheck = HrirValidator::validateFileSize(file.getSize());
    if (!sizeCheck.valid) {
      std::cout << json{{"ok", false}, {"error", sizeCheck.errorMessage}}.dump()
                << "\n";
      return 1;
    }

    WavHrirParser parser;
    auto result = parser.parse(file);

    if (!result.has_value()) {
      std::cout << json{{"ok", false}, {"error", result.error()}}.dump()
                << "\n";
      return 1;
    }

    // Serialise HrirSet to JSON
    const HrirSet &hs = *result;
    json data;
    data["name"] = hs.name.toStdString();
    data["source"] = hs.source.toStdString();
    data["channels"] = json::array();

    for (int s = 0; s < HrirSet::kSpeakerCount; ++s) {
      const auto &pair = hs.channels[s];
      json ch;
      ch["label"] = pair.label.toStdString();
      ch["sampleRate"] = pair.sampleRate;
      ch["irLen"] = pair.left.getNumSamples();

      // Serialise IR samples as base64 (simpler than individual floats)
      // Left ear
      {
        const int n = pair.left.getNumSamples();
        std::vector<float> v(pair.left.getReadPointer(0),
                             pair.left.getReadPointer(0) + n);
        ch["left"] = v; // nlohmann serialises float[] natively
      }
      // Right ear
      {
        const int n = pair.right.getNumSamples();
        std::vector<float> v(pair.right.getReadPointer(0),
                             pair.right.getReadPointer(0) + n);
        ch["right"] = v;
      }
      data["channels"].push_back(ch);
    }

    std::cout << json{{"ok", true}, {"hrirSet", data}}.dump() << "\n";
    return 0;

  } catch (const json::exception &e) {
    std::cout << json{{"ok", false},
                      {"error", std::string("JSON parse error: ") + e.what()}}
                     .dump()
              << "\n";
    return 1;
  } catch (const std::exception &e) {
    std::cout << json{{"ok", false},
                      {"error", std::string("Exception: ") + e.what()}}
                     .dump()
              << "\n";
    return 1;
  }
}

// ---------------------------------------------------------------------------
// Parent process: spawn subprocess and communicate
// ---------------------------------------------------------------------------
std::future<HrirParseResult> HrirParserWorker::parseAsync(
    const juce::File &file,
    std::function<void(HrirParseResult)> resultCallback) {
  return std::async(std::launch::async,
                    [this, file, cb = std::move(resultCallback)]() mutable {
                      auto result = parseSync(file);
                      if (cb) {
                        juce::MessageManager::callAsync(
                            [cb = std::move(cb), result = result]() mutable {
                              cb(std::move(result));
                            });
                      }
                      return result;
                    });
}

HrirParseResult HrirParserWorker::parseSync(const juce::File &file) {
  HrirParseRequest req;
  req.filePath = file.getFullPathName();
  req.format = file.hasFileExtension(".sofa") ? "sofa" : "wav";

  auto jsonLine = spawnAndParse(req);
  if (!jsonLine.has_value())
    return std::unexpected("Parser subprocess failed to start or timed out");

  return deserialiseResult(*jsonLine);
}

void HrirParserWorker::shutdown() noexcept {
  juce::ScopedLock lock(m_lock);
  if (m_childProcess) {
    m_childProcess->kill();
    m_childProcess.reset();
  }
  m_running.store(false, std::memory_order_relaxed);
}

std::optional<juce::String>
HrirParserWorker::spawnAndParse(const HrirParseRequest &req) {
  juce::ScopedLock lock(m_lock);

  // Get path to this executable (subprocess re-runs with special flag)
  juce::File exe =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile);

  m_childProcess = std::make_unique<juce::ChildProcess>();
  m_running.store(true, std::memory_order_relaxed);

  // Launch with --hrir-parser-worker flag
  juce::StringArray args;
  args.add(exe.getFullPathName());
  args.add("--hrir-parser-worker");

  if (!m_childProcess->start(args)) {
    m_running.store(false, std::memory_order_relaxed);
    return std::nullopt;
  }

  // Send request JSON to subprocess stdin
  json reqJson;
  reqJson["path"] = req.filePath.toStdString();
  reqJson["format"] = req.format.toStdString();
  reqJson["maxFileBytes"] = req.maxFileBytes;
  std::string requestLine = reqJson.dump() + "\n";
  m_childProcess->writeToStdin(requestLine.data(), requestLine.size());

  // Wait for response with timeout
  juce::String output;
  const int kPollMs = 50;
  const int kMaxPolls = kTimeoutMs / kPollMs;
  for (int poll = 0; poll < kMaxPolls; ++poll) {
    juce::Thread::sleep(kPollMs);
    output += m_childProcess->readAllProcessOutput();
    if (output.containsChar('\n'))
      break;
  }

  m_childProcess->waitForProcessToFinish(500);
  m_running.store(false, std::memory_order_relaxed);
  m_childProcess.reset();

  juce::String line = output.upToFirstOccurrenceOf("\n", false, false).trim();
  if (line.isEmpty())
    return std::nullopt;
  return line;
}

HrirParseResult
HrirParserWorker::deserialiseResult(const juce::String &jsonLine) {
  try {
    auto j = json::parse(jsonLine.toStdString());
    if (!j.value("ok", false))
      return std::unexpected(j.value("error", "Unknown parser error"));

    const auto &data = j["hrirSet"];
    HrirSet hrirSet;
    hrirSet.name = juce::String(data.value("name", ""));
    hrirSet.source = juce::String(data.value("source", ""));

    const auto &channels = data["channels"];
    for (int s = 0;
         s < HrirSet::kSpeakerCount && s < static_cast<int>(channels.size());
         ++s) {
      const auto &ch = channels[s];
      auto &pair = hrirSet.channels[s];
      pair.sampleRate = ch.value("sampleRate", 48000.0);
      pair.label = juce::String(ch.value("label", ""));

      std::vector<float> leftV = ch["left"].get<std::vector<float>>();
      std::vector<float> rightV = ch["right"].get<std::vector<float>>();

      pair.left.setSize(1, static_cast<int>(leftV.size()));
      pair.right.setSize(1, static_cast<int>(rightV.size()));
      std::copy(leftV.begin(), leftV.end(), pair.left.getWritePointer(0));
      std::copy(rightV.begin(), rightV.end(), pair.right.getWritePointer(0));

      auto valResult = HrirValidator::validateChannelPair(pair.left, pair.right,
                                                          pair.sampleRate);
      if (!valResult.valid)
        return std::unexpected("Subprocess returned invalid HRIR for " +
                               pair.label.toStdString() + ": " +
                               valResult.errorMessage);
    }

    return hrirSet;

  } catch (const std::exception &e) {
    return std::unexpected(std::string("Deserialise error: ") + e.what());
  }
}

} // namespace virtuoso

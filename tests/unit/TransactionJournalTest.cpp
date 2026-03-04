// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// TransactionJournalTest.cpp — Unit tests for TransactionJournal

#include "../../src/safety/TransactionJournal.h"
#include <catch2/catch_all.hpp>
#include <chrono>
#include <juce_core/juce_core.h>
#include <thread>


using namespace virtuoso;

// Helper: create a journal backed by a temp file
static juce::File makeTempJournalFile() {
  return juce::File::createTempFile(".journal.json");
}

TEST_CASE("TransactionJournal: basic state machine", "[safety][journal]") {
  auto file = makeTempJournalFile();
  TransactionJournal journal(file);

  SECTION("Initial state is Idle") {
    REQUIRE(journal.state() == TransactionJournal::State::Idle);
  }

  SECTION("Begin transitions to InProgress") {
    REQUIRE(journal.begin("test-op", {{"key", "value"}}));
    REQUIRE(journal.state() == TransactionJournal::State::InProgress);
  }

  SECTION("Commit transitions to Committed then Idle") {
    journal.begin("test-op", {});
    REQUIRE(journal.commit());
    REQUIRE(journal.state() == TransactionJournal::State::Idle);
  }

  SECTION("Rollback from InProgress transitions to RolledBack then Idle") {
    journal.begin("test-op", {});
    REQUIRE(journal.rollback("test rollback"));
    REQUIRE(journal.state() == TransactionJournal::State::Idle);
  }

  SECTION("Cannot begin while InProgress") {
    journal.begin("op1", {});
    REQUIRE_FALSE(journal.begin("op2", {}));
  }

  file.deleteFile();
}

TEST_CASE("TransactionJournal: crash recovery", "[safety][journal]") {
  auto file = makeTempJournalFile();
  juce::String txId;

  SECTION("Incomplete transaction detected on reload") {
    // Simulate crash: begin but never commit
    {
      TransactionJournal j1(file);
      j1.begin("crash-op", {{"data", "42"}});
      txId = j1.currentTransactionId();
      // j1 destructs without commit — simulates crash
    }

    // New instance detects the incomplete transaction
    TransactionJournal j2(file);
    REQUIRE(j2.hasIncompleteTransaction());
    REQUIRE(j2.incompleteTransactionId() == txId);
  }

  SECTION("Committed transaction not flagged as incomplete on reload") {
    {
      TransactionJournal j1(file);
      j1.begin("safe-op", {});
      j1.commit();
    }
    TransactionJournal j2(file);
    REQUIRE_FALSE(j2.hasIncompleteTransaction());
  }

  file.deleteFile();
}

TEST_CASE("ProfileSchema: JSON round-trip", "[profile]") {
  using namespace virtuoso;

  VirtuosoProfile orig = VirtuosoProfile::makeDefault();
  orig.displayName = "Test Profile";
  orig.masterVolumeDb = -6.0f;
  orig.eqEnabled = false;
  orig.limiterThresholdDb = -3.0f;

  // Serialise
  juce::String json = orig.toJsonString();
  REQUIRE_FALSE(json.isEmpty());

  // Deserialise
  auto loaded = VirtuosoProfile::fromJsonString(json);
  REQUIRE(loaded.has_value());
  REQUIRE(loaded->displayName == orig.displayName);
  REQUIRE(loaded->masterVolumeDb ==
          Catch::Approx(orig.masterVolumeDb).epsilon(0.001));
  REQUIRE(loaded->eqEnabled == orig.eqEnabled);
  REQUIRE(loaded->limiterThresholdDb ==
          Catch::Approx(orig.limiterThresholdDb).epsilon(0.001));
}

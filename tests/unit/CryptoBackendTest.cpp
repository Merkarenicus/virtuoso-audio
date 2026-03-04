// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// CryptoBackendTest.cpp — Unit tests for VirtuosoCrypto

#define CATCH_CONFIG_MAIN
#include "../../src/util/Crypto.h"
#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>
#include <vector>


using namespace virtuoso;

TEST_CASE("VirtuosoCrypto: round-trip encrypt/decrypt", "[crypto]") {
  const std::string plaintext =
      "Hello, Virtuoso Audio! This is a test. 1234567890 @#$%";
  const std::string_view context = "test.context";

  SECTION("Encrypt produces ciphertext different from plaintext") {
    auto ct = VirtuosoCrypto::encryptString(plaintext, context);
    REQUIRE(ct.has_value());
    const auto &ctVec = *ct;
    // Ciphertext must be different from plaintext
    bool identical =
        (ctVec.size() == plaintext.size()) &&
        std::equal(ctVec.begin(), ctVec.end(),
                   reinterpret_cast<const uint8_t *>(plaintext.data()));
    REQUIRE_FALSE(identical);
  }

  SECTION("Decrypt recovers original plaintext") {
    auto ct = VirtuosoCrypto::encryptString(plaintext, context);
    REQUIRE(ct.has_value());

    auto decrypted = VirtuosoCrypto::decryptToString(*ct, context);
    REQUIRE(decrypted.has_value());
    REQUIRE(*decrypted == plaintext);
  }

  SECTION("Decrypt with wrong context fails") {
    auto ct = VirtuosoCrypto::encryptString(plaintext, context);
    REQUIRE(ct.has_value());

    auto bad = VirtuosoCrypto::decryptToString(*ct, "wrong.context");
    REQUIRE_FALSE(bad.has_value());
  }

  SECTION("Tampered ciphertext fails authentication") {
    auto ct = VirtuosoCrypto::encryptString(plaintext, context);
    REQUIRE(ct.has_value());

    // Flip a byte in the ciphertext
    if (!ct->empty())
      (*ct)[ct->size() / 2] ^= 0xFF;

    auto bad = VirtuosoCrypto::decryptToString(*ct, context);
    REQUIRE_FALSE(bad.has_value());
  }

  SECTION("Empty plaintext round-trips correctly") {
    auto ct = VirtuosoCrypto::encryptString("", context);
    REQUIRE(ct.has_value());
    auto dec = VirtuosoCrypto::decryptToString(*ct, context);
    REQUIRE(dec.has_value());
    REQUIRE(dec->empty());
  }

  SECTION("Two encryptions of same plaintext produce different ciphertext "
          "(nonce randomisation)") {
    auto ct1 = VirtuosoCrypto::encryptString(plaintext, context);
    auto ct2 = VirtuosoCrypto::encryptString(plaintext, context);
    REQUIRE(ct1.has_value());
    REQUIRE(ct2.has_value());
    // XChaCha20-Poly1305 uses a random 192-bit nonce — they should differ
    REQUIRE(*ct1 != *ct2);
  }

  SECTION("Large payload round-trips") {
    std::string bigPayload(1024 * 1024, 'A'); // 1 MB
    auto ct = VirtuosoCrypto::encryptString(bigPayload, context);
    REQUIRE(ct.has_value());
    auto dec = VirtuosoCrypto::decryptToString(*ct, context);
    REQUIRE(dec.has_value());
    REQUIRE(*dec == bigPayload);
  }
}

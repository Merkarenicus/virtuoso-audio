// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// Crypto.h
// Unified authenticated encryption interface.
// Platform backends per ADR-004:
//   Windows: Windows CNG (BCrypt) — AES-256-GCM
//   macOS:   CommonCrypto — AES-256-GCM
//   Linux (libsecret): libsodium — XChaCha20-Poly1305
//   Linux fallback: libsodium — key stored in
//   ~/.config/virtuoso-audio/.keystore

#pragma once
#include <juce_core/juce_core.h>
#include <optional>
#include <span>
#include <vector>


namespace virtuoso {

class VirtuosoCrypto {
public:
  // -------------------------------------------------------------------------------------
  // Encrypt plaintext → ciphertext (authenticated: includes MAC tag).
  // context_label: arbitrary string binding the ciphertext to a purpose (key
  // derivation) Returns std::nullopt on failure (never throws).
  // -------------------------------------------------------------------------------------
  static std::optional<std::vector<uint8_t>>
  encrypt(std::span<const uint8_t> plaintext,
          std::string_view context_label) noexcept;

  // Decrypt ciphertext → plaintext.
  // Returns std::nullopt if:
  //   - Ciphertext is truncated
  //   - Authentication tag mismatch (tamper detected)
  //   - Key not found in OS keystore
  static std::optional<std::vector<uint8_t>>
  decrypt(std::span<const uint8_t> ciphertext,
          std::string_view context_label) noexcept;

  // Convenience: encrypt/decrypt juce::String (UTF-8)
  static std::optional<std::vector<uint8_t>>
  encryptString(const juce::String &plaintext,
                std::string_view context_label) noexcept;

  static std::optional<juce::String>
  decryptToString(std::span<const uint8_t> ciphertext,
                  std::string_view context_label) noexcept;

  // Returns true if the current platform has a secure keystore available
  static bool hasSecureKeystore() noexcept;

  // Securely zero a byte range (cannot be optimised out by compiler)
  static void secureZero(void *ptr, size_t size) noexcept;

private:
  // Platform-specific key derivation / retrieval
  static std::optional<std::vector<uint8_t>>
  getOrCreateKey(std::string_view context_label) noexcept;

#if defined(_WIN32)
  // Windows CNG (BCrypt) AES-256-GCM
  static std::optional<std::vector<uint8_t>>
  encryptWindows(std::span<const uint8_t> plaintext,
                 std::span<const uint8_t> key) noexcept;
  static std::optional<std::vector<uint8_t>>
  decryptWindows(std::span<const uint8_t> ciphertext,
                 std::span<const uint8_t> key) noexcept;

#elif defined(__APPLE__)
  // macOS CommonCrypto AES-256-GCM
  static std::optional<std::vector<uint8_t>>
  encryptApple(std::span<const uint8_t> plaintext,
               std::span<const uint8_t> key) noexcept;
  static std::optional<std::vector<uint8_t>>
  decryptApple(std::span<const uint8_t> ciphertext,
               std::span<const uint8_t> key) noexcept;

#else
  // Linux: libsodium XChaCha20-Poly1305
  static std::optional<std::vector<uint8_t>>
  encryptSodium(std::span<const uint8_t> plaintext,
                std::span<const uint8_t> key) noexcept;
  static std::optional<std::vector<uint8_t>>
  decryptSodium(std::span<const uint8_t> ciphertext,
                std::span<const uint8_t> key) noexcept;
#endif
};

} // namespace virtuoso

# ADR-004: Profile Encryption & Crypto Backend

**Status**: Accepted  
**Date**: 2026-03-04  
**Primary source**: Codex review (confirmed by DeepSeek, Grok)

---

## Context

The original plan specified "AES-256-GCM profile encryption using tiny-AES." This is incorrect:

**tiny-AES does NOT support AES-GCM.** The library only implements ECB, CBC, and CTR modes. GCM requires a GHASH authentication tag, which tiny-AES completely lacks. Using CTR mode without authentication would provide confidentiality but NO integrity protection — a tampered profile would not be detected.

Additionally, using any homebrew or minimal crypto library for key management in a desktop application introduces unnecessary risk. Key storage (where does the key for the encrypted blob live?) is the harder problem than the encryption itself.

## Decision

**Replace tiny-AES with OS-native authenticated encryption using platform keystores for key management.**

### Encryption Backend

| Platform | Encryption API | Key Storage |
|---|---|---|
| Windows | `BCryptEncrypt` (Windows CNG) with AES-256-GCM | DPAPI (`CryptProtectData`) — per-user, bound to Windows login |
| macOS | `CCCryptorCreateWithMode` (CommonCrypto) with AES-256-GCM | Keychain Services (`SecItemAdd`) |
| Linux (libsecret available) | libsodium `crypto_aead_xchacha20poly1305_ietf` | libsecret / GNOME Keyring / KWallet |
| Linux (no libsecret) | libsodium `crypto_aead_xchacha20poly1305_ietf` | Key stored in `~/.config/virtuoso-audio/.keystore` (plaintext fallback, warning shown in UI) |

### Why libsodium on Linux?

libsodium's `XChaCha20-Poly1305` is a modern AEAD cipher that:
- Provides authenticated encryption (tamper detection)
- Has a large 192-bit nonce (safe for random generation without collision risk)
- Is implemented by a widely audited, fuzz-tested library
- Does not require hardware AES support (safe on all ARM/x86 Linux targets)

### Profile data that is encrypted

The profile JSON stores:
- User display name, EQ settings, HRIR selection paths (NOT encrypted — these are preferences)
- `device_fingerprint`: a hash of the audio device IDs (encrypted — used to bind profiles to physical devices)

Only the `device_fingerprint` field is encrypted. The rest of the profile is stored as readable JSON. This avoids the entire "where does the key go?" problem for most data, while still protecting the identifying information.

### Key Rotation

On Windows: DPAPI keys are automatically rotated by Windows when the user changes their password. No application-level key rotation is needed.  
On macOS: Keychain access is gated by the app's code signature. No rotation needed unless the signing identity changes.  
On Linux: A warning is shown and re-encryption with a new key is offered whenever the app detects that the libsecret item has been deleted.

## Consequences

- `tiny-AES` removed from all dependency lists and CMakeLists
- `libsodium` added as a CPM dependency
- `src/util/Crypto.h/.cpp` provides the unified `VirtuosoCrypto` interface:
  ```cpp
  class VirtuosoCrypto {
  public:
      // Returns empty on failure; never throws
      static std::optional<std::vector<uint8_t>> encrypt(
          std::span<const uint8_t> plaintext,
          std::string_view context_label);  // used for DPAPI/Keychain context
      static std::optional<std::vector<uint8_t>> decrypt(
          std::span<const uint8_t> ciphertext,
          std::string_view context_label);
  };
  ```
- `ProfileEncryption.h/.cpp` uses `VirtuosoCrypto` and is tested with known-answer tests (KATs)
- CI unit test `CryptoBackendTest`:
  1. Encrypt a known plaintext
  2. Assert ciphertext does not contain plaintext bytes
  3. Decrypt and assert round-trip equality
  4. Tamper with one byte of ciphertext; assert decryption fails (authentication tag check)

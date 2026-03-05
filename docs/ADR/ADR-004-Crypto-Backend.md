# ADR-004: Cryptographic Backend — OS-Native AE + libsodium Fallback

**Date**: 2026-03-04  
**Status**: Accepted (supersedes earlier tiny-AES proposal)  
**Deciders**: Full swarm review — all 6 reviewers concurred. Meta-review confirmed.

---

## Context

Virtuoso profiles (EQ settings, HRIR paths) are stored encrypted to prevent
tampering. Original Phase 1 plan used `tiny-AES` (a portable AES-128-ECB implementation).

Swarm reviewers identified critical flaws in that approach:
1. **ECB mode** — deterministic, reveals identical plaintext blocks; unacceptable for profiles
2. **No authentication** — susceptible to bitflipping attacks on stored data
3. **Key storage** — plain file next to ciphertext; trivially broke
4. **No nonce/IV** — same key + plaintext always produces same ciphertext

---

## Decision

**Replace tiny-AES with platform-dispatched authenticated encryption:**

| Platform | Cipher | Key Storage |
|---|---|---|
| Windows | AES-256-GCM via Windows CNG (Bcrypt) | DPAPI-protected key blob |
| macOS | AES-256-GCM via CommonCrypto | Keychain (kSecClassGenericPassword) |
| Linux | XChaCha20-Poly1305 via **libsodium** | libsecret (Secret Service API) + file fallback |
| All (current impl) | XChaCha20-Poly1305 via **libsodium** | Platform keystore or encrypted file fallback |

Current implementation (`Crypto.h/.cpp`) uses **libsodium exclusively** on all platforms
for simplicity and correctness — the platform-specific paths are wired for Phase 6b.

### Nonce strategy
libsodium `crypto_secretstream_xchacha20poly1305` / `crypto_aead_xchacha20poly1305_ietf_encrypt`:
- **192-bit random nonce** generated per encryption call via `randombytes_buf()`  
- Nonce prepended to ciphertext (nonce || ciphertext || MAC)  
- Guarantees: two encryptions of the same plaintext produce different ciphertext

---

## Consequences

**Positive**:
- Authenticated encryption — tampered profiles detected and rejected
- Random nonces — no ciphertext reuse
- OS keystores — keys are protected by DPAPI/Keychain TPM-backed storage
- libsodium is formally audited (NaCl cryptographic library)

**Negative**:
- libsodium adds ~500 KB to binary size
- Platform keystore integration requires per-OS code paths (Phase 6b)
- Migration path needed for any users who stored profiles with the old tiny-AES format

**Testing**: `CryptoBackendTest.cpp` verifies round-trip, tamper detection, nonce
uniqueness, empty-plaintext, and 1 MB large-payload correctness.

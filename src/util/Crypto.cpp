// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// Crypto.cpp — Platform-dispatched authenticated encryption
// See ADR-004 for rationale.

#include "Crypto.h"
#include <cstring>
#include <sodium.h> // Always available (linked on all platforms)


#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <dpapi.h>
#include <wincrypt.h>
#include <windows.h>
#pragma comment(lib, "crypt32.lib")
#elif defined(__APPLE__)
#include <CommonCrypto/CommonCryptor.h>
#include <Security/Security.h>
#endif

namespace virtuoso {

// ---------------------------------------------------------------------------
// Secure zeroing (cannot be optimised away)
// ---------------------------------------------------------------------------
void VirtuosoCrypto::secureZero(void *ptr, size_t size) noexcept {
  sodium_memzero(ptr, size);
}

// ---------------------------------------------------------------------------
// hasSecureKeystore
// ---------------------------------------------------------------------------
bool VirtuosoCrypto::hasSecureKeystore() noexcept {
#if defined(_WIN32) || defined(__APPLE__)
  return true;
#else
  // Check for libsecret / GNOME keyring at runtime
  return false; // Safe fallback — Crypto.cpp Phase 3 will add libsecret query
#endif
}

// ---------------------------------------------------------------------------
// Key management: get/create a 32-byte key stored in OS keystore
// ---------------------------------------------------------------------------
std::optional<std::vector<uint8_t>>
VirtuosoCrypto::getOrCreateKey(std::string_view context_label) noexcept {
#if defined(_WIN32)
  // Windows: use DPAPI to protect a randomly generated 32-byte key.
  // The key is stored in the user profile — automatically tied to Windows
  // login. We use a per-context "entropy" blob derived from context_label so
  // each context gets a distinct key via DPAPI's optional entropy parameter.

  std::vector<uint8_t> entropy(context_label.begin(), context_label.end());

  // Generate a 32-byte random key (first run), then protect with DPAPI
  // Subsequent runs: check registry for protected blob, then unprotect
  constexpr WCHAR kKeyPath[] = L"SOFTWARE\\VirtuosoAudio\\Keys";
  HKEY hKey = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, kKeyPath, 0, nullptr,
                      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey,
                      nullptr) != ERROR_SUCCESS)
    return std::nullopt;

  std::wstring valueName(context_label.begin(), context_label.end());
  DWORD dataSize = 0;
  bool hasKey = (RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr,
                                  nullptr, &dataSize) == ERROR_SUCCESS);

  std::vector<uint8_t> protectedBlob;
  if (!hasKey) {
    // First run: generate random key and protect with DPAPI
    std::vector<uint8_t> rawKey(32);
    randombytes_buf(rawKey.data(), 32);

    DATA_BLOB dataIn{32, rawKey.data()};
    DATA_BLOB entropyBlob{static_cast<DWORD>(entropy.size()), entropy.data()};
    DATA_BLOB dataOut{};

    if (!CryptProtectData(&dataIn, L"VirtuosoAudio", &entropyBlob, nullptr,
                          nullptr, CRYPTPROTECT_UI_FORBIDDEN, &dataOut)) {
      RegCloseKey(hKey);
      secureZero(rawKey.data(), rawKey.size());
      return std::nullopt;
    }
    protectedBlob.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
    LocalFree(dataOut.pbData);
    secureZero(rawKey.data(), rawKey.size());

    RegSetValueExW(hKey, valueName.c_str(), 0, REG_BINARY, protectedBlob.data(),
                   static_cast<DWORD>(protectedBlob.size()));
  } else {
    protectedBlob.resize(dataSize);
    RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr,
                     protectedBlob.data(), &dataSize);
  }
  RegCloseKey(hKey);

  // Unprotect to get raw key
  DATA_BLOB dataIn2{static_cast<DWORD>(protectedBlob.size()),
                    protectedBlob.data()};
  DATA_BLOB entropyBlob2{static_cast<DWORD>(entropy.size()), entropy.data()};
  DATA_BLOB dataOut2{};

  if (!CryptUnprotectData(&dataIn2, nullptr, &entropyBlob2, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &dataOut2))
    return std::nullopt;

  std::vector<uint8_t> key(dataOut2.pbData, dataOut2.pbData + dataOut2.cbData);
  LocalFree(dataOut2.pbData);
  return key;

#elif defined(__APPLE__)
  // macOS: Keychain Services — store a 32-byte random key as a generic password
  // item
  std::string service = "audio.virtuoso.app";
  std::string account(context_label);

  // Try to read existing key from Keychain
  CFStringRef cfService = CFStringCreateWithCString(nullptr, service.c_str(),
                                                    kCFStringEncodingUTF8);
  CFStringRef cfAccount = CFStringCreateWithCString(nullptr, account.c_str(),
                                                    kCFStringEncodingUTF8);

  CFDataRef keyData = nullptr;
  const void *keys[] = {kSecClass, kSecAttrService, kSecAttrAccount,
                        kSecReturnData, kSecMatchLimit};
  const void *vals[] = {kSecClassGenericPassword, cfService, cfAccount,
                        kCFBooleanTrue, kSecMatchLimitOne};
  CFDictionaryRef query =
      CFDictionaryCreate(nullptr, keys, vals, 5, &kCFTypeDictionaryKeyCallBacks,
                         &kCFTypeDictionaryValueCallBacks);
  OSStatus status = SecItemCopyMatching(query, (CFTypeRef *)&keyData);
  CFRelease(query);
  CFRelease(cfService);
  CFRelease(cfAccount);

  if (status == errSecSuccess && keyData) {
    std::vector<uint8_t> key(CFDataGetLength(keyData));
    std::memcpy(key.data(), CFDataGetBytePtr(keyData), key.size());
    CFRelease(keyData);
    return key;
  }

  // Not found: generate and store
  std::vector<uint8_t> rawKey(32);
  randombytes_buf(rawKey.data(), 32);

  CFStringRef cfService2 = CFStringCreateWithCString(nullptr, service.c_str(),
                                                     kCFStringEncodingUTF8);
  CFStringRef cfAccount2 = CFStringCreateWithCString(nullptr, account.c_str(),
                                                     kCFStringEncodingUTF8);
  CFDataRef cfKey = CFDataCreate(nullptr, rawKey.data(), 32);
  const void *addKeys[] = {kSecClass, kSecAttrService, kSecAttrAccount,
                           kSecValueData};
  const void *addVals[] = {kSecClassGenericPassword, cfService2, cfAccount2,
                           cfKey};
  CFDictionaryRef addQ = CFDictionaryCreate(nullptr, addKeys, addVals, 4,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
  SecItemAdd(addQ, nullptr);
  CFRelease(addQ);
  CFRelease(cfService2);
  CFRelease(cfAccount2);
  CFRelease(cfKey);

  return rawKey;

#else
  // Linux: libsodium random key stored in
  // ~/.config/virtuoso-audio/.keystore/<context> (plaintext fallback — see
  // ADR-004. Phase 3 will add libsecret.)
  juce::File keyDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("virtuoso-audio/.keystore");
  keyDir.createDirectory();

  juce::String safeLabel =
      juce::String(context_label.data(), context_label.size())
          .replaceCharacters("/\\:*?\"<>|", "_________");
  juce::File keyFile = keyDir.getChildFile(safeLabel + ".key");

  if (keyFile.existsAsFile() && keyFile.getSize() == 32) {
    juce::MemoryBlock mb;
    keyFile.loadFileAsData(mb);
    return std::vector<uint8_t>(static_cast<uint8_t *>(mb.getData()),
                                static_cast<uint8_t *>(mb.getData()) + 32);
  }

  // Generate new key
  std::vector<uint8_t> key(32);
  randombytes_buf(key.data(), 32);
  keyFile.replaceWithData(key.data(), 32);
  return key;
#endif
}

// ---------------------------------------------------------------------------
// Encrypt / Decrypt — libsodium XChaCha20-Poly1305 on all platforms
// (Simple, well-audited, works everywhere. Native CNG/CommonCrypto are used
//  only for key storage, not for the actual cipher, to avoid complex FFI.)
// ---------------------------------------------------------------------------
std::optional<std::vector<uint8_t>>
VirtuosoCrypto::encrypt(std::span<const uint8_t> plaintext,
                        std::string_view context_label) noexcept {
  if (sodium_init() < 0)
    return std::nullopt;

  auto key = getOrCreateKey(context_label);
  if (!key.has_value() ||
      key->size() < crypto_aead_xchacha20poly1305_ietf_KEYBYTES)
    return std::nullopt;

  // Generate random nonce (192 bits = safe for random use)
  std::vector<uint8_t> nonce(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  randombytes_buf(nonce.data(), nonce.size());

  // Output: nonce || ciphertext+tag
  const size_t ciphertextLen =
      plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES;
  std::vector<uint8_t> output;
  output.reserve(nonce.size() + ciphertextLen);
  output.insert(output.end(), nonce.begin(), nonce.end());
  output.resize(nonce.size() + ciphertextLen);

  unsigned long long actualLen = 0;
  int rc = crypto_aead_xchacha20poly1305_ietf_encrypt(
      output.data() + nonce.size(), &actualLen, plaintext.data(),
      plaintext.size(), nullptr, 0, // no additional data
      nullptr,                      // nsec (unused in this construction)
      nonce.data(), key->data());

  secureZero(key->data(), key->size());
  if (rc != 0)
    return std::nullopt;

  output.resize(nonce.size() + static_cast<size_t>(actualLen));
  return output;
}

std::optional<std::vector<uint8_t>>
VirtuosoCrypto::decrypt(std::span<const uint8_t> ciphertext,
                        std::string_view context_label) noexcept {
  if (sodium_init() < 0)
    return std::nullopt;

  constexpr size_t kNonceLen = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
  constexpr size_t kTagLen = crypto_aead_xchacha20poly1305_ietf_ABYTES;

  if (ciphertext.size() < kNonceLen + kTagLen)
    return std::nullopt; // Too short to be valid

  auto key = getOrCreateKey(context_label);
  if (!key.has_value())
    return std::nullopt;

  const uint8_t *nonce = ciphertext.data();
  const uint8_t *cipherBody = ciphertext.data() + kNonceLen;
  const size_t bodyLen = ciphertext.size() - kNonceLen;

  std::vector<uint8_t> plaintext(bodyLen - kTagLen);
  unsigned long long ptLen = 0;

  int rc = crypto_aead_xchacha20poly1305_ietf_decrypt(
      plaintext.data(), &ptLen, nullptr, cipherBody, bodyLen, nullptr, 0, nonce,
      key->data());

  secureZero(key->data(), key->size());
  if (rc != 0) {
    secureZero(plaintext.data(), plaintext.size());
    return std::nullopt; // Authentication failed — tampered ciphertext
  }

  plaintext.resize(static_cast<size_t>(ptLen));
  return plaintext;
}

std::optional<std::vector<uint8_t>>
VirtuosoCrypto::encryptString(const juce::String &plaintext,
                              std::string_view context_label) noexcept {
  std::string utf8 = plaintext.toStdString();
  return encrypt({reinterpret_cast<const uint8_t *>(utf8.data()), utf8.size()},
                 context_label);
}

std::optional<juce::String>
VirtuosoCrypto::decryptToString(std::span<const uint8_t> ciphertext,
                                std::string_view context_label) noexcept {
  auto pt = decrypt(ciphertext, context_label);
  if (!pt.has_value())
    return std::nullopt;
  return juce::String::fromUTF8(reinterpret_cast<const char *>(pt->data()),
                                static_cast<int>(pt->size()));
}

} // namespace virtuoso

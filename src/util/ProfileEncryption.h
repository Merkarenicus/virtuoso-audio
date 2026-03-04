// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// ProfileEncryption.h — Thin wrapper combining VirtuosoCrypto with
// ProfileSchema

#pragma once
#include "../util/Crypto.h"
#include "ProfileSchema.h"
#include <optional>

namespace virtuoso {

// This class centralises all profile encryption/decryption in one place
// and is separate from ProfileManager so it can be unit-tested independently.
class ProfileEncryption {
public:
  static constexpr std::string_view kContextLabel = "virtuoso.profile";

  static std::optional<std::vector<uint8_t>>
  encrypt(const VirtuosoProfile &profile) {
    return VirtuosoCrypto::encryptString(profile.toJsonString(), kContextLabel);
  }

  static std::optional<VirtuosoProfile>
  decrypt(std::span<const uint8_t> ciphertext) {
    auto json = VirtuosoCrypto::decryptToString(ciphertext, kContextLabel);
    if (!json.has_value())
      return std::nullopt;
    return VirtuosoProfile::fromJsonString(*json);
  }
};

} // namespace virtuoso

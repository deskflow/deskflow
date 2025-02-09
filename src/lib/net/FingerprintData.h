/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace deskflow {

enum FingerprintType
{
  Invalid,
  SHA1,
  SHA256
};

struct FingerprintData
{
  std::string algorithm;
  std::vector<std::uint8_t> data;

  bool valid() const
  {
    return !algorithm.empty();
  }

  bool operator==(const FingerprintData &other) const;
};

const char *fingerprintTypeToString(FingerprintType type);
FingerprintType fingerprintTypeFromString(const std::string &type);

} // namespace deskflow

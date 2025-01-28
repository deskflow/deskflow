/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FingerprintDatabase.h"

#include "io/filesystem.h"

#include <algorithm>
#include <fstream>

namespace deskflow {

bool FingerprintData::operator==(const FingerprintData &other) const
{
  return algorithm == other.algorithm && data == other.data;
}

const char *fingerprintTypeToString(FingerprintType type)
{
  switch (type) {
  case FingerprintType::Invalid:
    return "invalid";
  case FingerprintType::SHA1:
    return "sha1";
  case FingerprintType::SHA256:
    return "sha256";
  default:
    break;
  }
  return "invalid";
}

FingerprintType fingerprintTypeFromString(const std::string &type)
{
  if (type == "sha1")
    return FingerprintType::SHA1;

  if (type == "sha256")
    return FingerprintType::SHA256;

  return FingerprintType::Invalid;
}

} // namespace deskflow

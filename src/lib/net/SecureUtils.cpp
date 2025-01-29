/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureUtils.h"
#include "base/String.h"

namespace deskflow {

std::string formatSSLFingerprint(const std::vector<uint8_t> &fingerprint, bool enableSeparators)
{
  std::string result = deskflow::string::toHex(fingerprint, 2);

  deskflow::string::uppercase(result);

  if (enableSeparators) {
    const auto usedSpaces = 3;
    size_t separators = result.size() / 2;
    for (size_t i = 1; i < separators; i++)
      result.insert(i * usedSpaces - 1, ":");
  }
  return result;
}

} // namespace deskflow

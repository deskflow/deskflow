/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Devlopers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureUtils.h"
#include "base/String.h"

namespace deskflow {

std::string formatSSLFingerprint(const std::vector<uint8_t> &fingerprint, bool enableSeparators)
{
  std::string rtn = deskflow::string::toHex(fingerprint, 2);

  deskflow::string::uppercase(rtn);

  if (enableSeparators) {
    size_t separators = rtn.size() / 2;
    for (size_t i = 1; i < separators; i++)
      rtn.insert(i * 3 - 1, ":");
  }
  return rtn;
}

} // namespace deskflow

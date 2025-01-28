/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "FingerprintData.h"

#include "io/filesystem.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace deskflow {

class FingerprintDatabase
{
public:
  void read(const fs::path &path);
  void write(const fs::path &path);

  void readStream(std::istream &stream);
  void writeStream(std::ostream &stream);

  void clear();
  void addTrusted(const FingerprintData &fingerprint);
  bool isTrusted(const FingerprintData &fingerprint);

  const std::vector<FingerprintData> &fingerprints() const
  {
    return m_fingerprints;
  }

  static FingerprintData parseDbLine(const std::string &line);
  static std::string toDbLine(const FingerprintData &fingerprint);

private:
  std::vector<FingerprintData> m_fingerprints;
};

} // namespace deskflow

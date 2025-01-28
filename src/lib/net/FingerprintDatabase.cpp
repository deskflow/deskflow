/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FingerprintDatabase.h"

#include "base/String.h"
#include "io/filesystem.h"

#include <algorithm>
#include <fstream>

namespace deskflow {

void FingerprintDatabase::read(const fs::path &path)
{
  std::ifstream file;
  openUtf8Path(file, path, std::ios_base::in);
  readStream(file);
}

void FingerprintDatabase::write(const fs::path &path)
{
  std::ofstream file;
  openUtf8Path(file, path, std::ios_base::out);
  writeStream(file);
}

void FingerprintDatabase::readStream(std::istream &stream)
{
  if (!stream.good()) {
    return;
  }

  std::string line;
  while (std::getline(stream, line)) {
    if (line.empty()) {
      continue;
    }

    auto fingerprint = parseDbLine(line);
    if (!fingerprint.valid()) {
      continue;
    }

    m_fingerprints.push_back(fingerprint);
  }
}

void FingerprintDatabase::writeStream(std::ostream &stream)
{
  if (!stream.good()) {
    return;
  }

  for (const auto &fingerprint : m_fingerprints) {
    stream << toDbLine(fingerprint) << "\n";
  }
}

void FingerprintDatabase::clear()
{
  m_fingerprints.clear();
}

void FingerprintDatabase::addTrusted(const FingerprintData &fingerprint)
{
  if (isTrusted(fingerprint)) {
    return;
  }
  m_fingerprints.push_back(fingerprint);
}

bool FingerprintDatabase::isTrusted(const FingerprintData &fingerprint)
{
  auto found = std::find(m_fingerprints.begin(), m_fingerprints.end(), fingerprint);
  return found != m_fingerprints.end();
}

FingerprintData FingerprintDatabase::parseDbLine(const std::string &line)
{

  const auto kSha1ColonCount = 19;
  const auto kSha1HexCharCount = 40;
  const auto kSha1ExpectedSize = kSha1HexCharCount + kSha1ColonCount;

  FingerprintData result;

  // legacy v1 certificate handling
  if (std::count(line.begin(), line.end(), ':') == kSha1ColonCount && line.size() == kSha1ExpectedSize) {
    auto data = string::fromHex(line);
    if (data.empty()) {
      return result;
    }
    result.algorithm = fingerprintTypeToString(FingerprintType::SHA1);
    result.data = data;
    return result;
  }

  auto versionEndPos = line.find(':');
  if (versionEndPos == std::string::npos) {
    return result;
  }
  if (line.substr(0, versionEndPos) != "v2") {
    return result;
  }
  auto algoStartPos = versionEndPos + 1;
  auto algoEndPos = line.find(':', algoStartPos);
  if (algoEndPos == std::string::npos) {
    return result;
  }
  auto algorithm = line.substr(algoStartPos, algoEndPos - algoStartPos);
  auto data = string::fromHex(line.substr(algoEndPos + 1));

  if (data.empty()) {
    return result;
  }

  result.algorithm = algorithm;
  result.data = data;
  return result;
}

std::string FingerprintDatabase::toDbLine(const FingerprintData &fingerprint)
{
  std::string fingerprintStr(fingerprint.data.begin(), fingerprint.data.end());
  return "v2:" + fingerprint.algorithm + ":" + string::toHex(fingerprintStr, 2);
}

} // namespace deskflow

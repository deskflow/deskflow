/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Fingerprint.h"

#include <QDebug>

bool Fingerprint::isValid() const
{
  switch (type) {
  case Fingerprint::Type::Invalid:
    return false;
  case Fingerprint::Type::SHA1:
    return data.length() == 20;
  case Fingerprint::Type::SHA256:
    return data.length() == 32;
  default:
    return false;
  }
}

bool Fingerprint::operator==(const Fingerprint &other) const
{
  return type == other.type && data == other.data;
}

QString Fingerprint::toDbLine() const
{
  if (!isValid()) {
    qInfo() << "unable to parse invalid db line";
    return {};
  }
  return QStringLiteral("v2:%1:%2").arg(typeToString(type), data.toHex().toLower());
}

Fingerprint Fingerprint::fromDbLine(const QString &line)
{
  Fingerprint result;

  if (line.isEmpty())
    return result;

  if (line.startsWith("v2:")) {
    const auto sLine = line.split(':');
    if (sLine.count() != 3)
      return result;
    result.type = typeFromString(sLine.at(1));
    result.data = QByteArray::fromHex(sLine.at(2).toLower().toLatin1());
  } else {
    // v1 fallback
    const auto kSha1ColonCount = 19;
    const auto kSha1HexCharCount = 40;
    const auto kSha1ExpectedSize = kSha1HexCharCount + kSha1ColonCount;
    if (line.size() != kSha1ExpectedSize || line.count(':') != kSha1ColonCount)
      return result;
    result.type = Fingerprint::Type::SHA1;
    auto l2 = line;
    result.data = QByteArray::fromHex(l2.remove(':').toLatin1());
  }

  return result;
}

Fingerprint::Type Fingerprint::typeFromString(const QString &type)
{
  const auto t = type.toLower();
  if (t == m_type_sha1)
    return Type::SHA1;
  if (t == m_type_sha256)
    return Type::SHA256;
  return Type::Invalid;
}

QString Fingerprint::typeToString(Fingerprint::Type type)
{
  switch (type) {
  case Type::SHA1:
    return m_type_sha1;
  case Type::SHA256:
    return m_type_sha256;
  default:
    return m_type_invalid;
  }
}

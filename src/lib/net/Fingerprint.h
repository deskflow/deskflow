/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QByteArray>
#include <QObject>

struct Fingerprint
{
private:
  Q_GADGET
  inline static QString m_type_sha1 = QStringLiteral("sha1");
  inline static QString m_type_sha256 = QStringLiteral("sha256");
  inline static QString m_type_invalid = QStringLiteral("invalid");

public:
  enum class Type
  {
    Invalid,
    SHA1,
    SHA256
  };
  Q_ENUM(Type)
  Type type = Type::Invalid;
  QByteArray data;

  bool isValid() const
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
  bool operator==(const Fingerprint &other) const;
  QString toDbLine() const;
  static Fingerprint fromDbLine(const QString &line);
  static QString typeToString(Fingerprint::Type type);
  static Fingerprint::Type typeFromString(const QString &type);
};

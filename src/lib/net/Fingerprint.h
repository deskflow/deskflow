/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QByteArray>
#include <QObject>

using namespace Qt::StringLiterals;

struct Fingerprint
{
  Q_GADGET
  inline static QString m_type_sha1 = u"sha1"_s;
  inline static QString m_type_sha256 = u"sha256"_s;
  inline static QString m_type_invalid = u"invalid"_s;

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

  bool isValid() const;

  bool operator==(const Fingerprint &other) const = default;
  QString toDbLine() const;
  static Fingerprint fromDbLine(const QString &line);
  static QString typeToString(Fingerprint::Type type);
  static Fingerprint::Type typeFromString(const QString &type);
};

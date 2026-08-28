/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QObject>

struct Fingerprint
{
  Q_GADGET
  inline static QString m_type_sha1 = QStringLiteral("sha1");
  inline static QString m_type_sha256 = QStringLiteral("sha256");
  inline static QString m_type_invalid = QStringLiteral("invalid");

public:
  // Since there is no "undefined" or "invalid" we will use MD4 the value of 0 as default.
  // Any type that is not Sha1 or Sha256 will be considered invalid
  QCryptographicHash::Algorithm type = QCryptographicHash::Md4;
  QByteArray data;

  bool isValid() const;

  bool operator==(const Fingerprint &other) const = default;
  QString toDbLine() const;
  static Fingerprint fromDbLine(const QString &line);
  static QString typeToString(QCryptographicHash::Algorithm type);
  static QCryptographicHash::Algorithm typeFromString(const QString &type);
};

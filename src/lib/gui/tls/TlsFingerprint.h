/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

class TlsFingerprint
{
private:
  explicit TlsFingerprint(const QString &filename);

public:
  static TlsFingerprint local();
  static TlsFingerprint trustedServers();
  static TlsFingerprint trustedClients();
  static QString directoryPath();
  static QString localFingerprint();
  static bool localFingerprintExists();
  static void persistDirectory();

  void trust(const QString &fingerprintText, bool append = true) const;
  bool isTrusted(const QString &fingerprintText) const;
  QStringList readList(const int readTo = -1) const;
  QString readFirst() const;
  QString filePath() const;
  bool fileExists() const;

private:
  QString m_Filename;
};

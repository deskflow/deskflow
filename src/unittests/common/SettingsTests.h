/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "../../lib/common/Settings.h"

#include <QTest>

class SettingsTests : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();
  // Test are run in order top to bottom
  void setSettingsFile();
  void settingsFile();
  void settingsPath();
  void tlsDir();
  void tlsLocalDb();
  void tlsTrustedServersDb();
  void tlsTrustedClientsDb();
  void checkValidSettings();

private:
  inline static const QString m_settingsPath = QStringLiteral("tmp/test");
  inline static const QString m_settingsFile = QStringLiteral("%1/Deskflow.conf").arg(m_settingsPath);
  inline static const QString m_expectedTlsDir = QStringLiteral("tmp/test/%1").arg(kTlsDirName);
  inline static const QString m_expectedTlsLocalDB =
      QStringLiteral("%1/%2").arg(m_expectedTlsDir, kTlsFingerprintLocalFilename);
  inline static const QString m_expectedTlsServerDB =
      QStringLiteral("%1/%2").arg(m_expectedTlsDir, kTlsFingerprintTrustedServersFilename);
  inline static const QString m_expectedTlsClientDB =
      QStringLiteral("%1/%2").arg(m_expectedTlsDir, kTlsFingerprintTrustedClientsFilename);
};

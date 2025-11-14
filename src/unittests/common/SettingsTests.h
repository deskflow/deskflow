/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "common/Settings.h"

#include <QTest>

class SettingsTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  // Test are run in order top to bottom
  void setSettingsFile();
  void setStateFile();
  void settingsFile();
  void settingsPath();
  void tlsDir();
  void tlsLocalDb();
  void tlsTrustedServersDb();
  void tlsTrustedClientsDb();
  void checkValidSettings();
  void checkCleanScreenName();
  void checkCleanScreenName_LongName();

private:
  inline static const QString m_settingsPathTemp = QStringLiteral("tmp/test");
  inline static const QString m_settingsFile = QStringLiteral("%1/Deskflow.conf").arg(m_settingsPathTemp);
  inline static const QString m_stateFile = QStringLiteral("%1/Deskflow.state").arg(m_settingsPathTemp);

// Gotcha: On Windows non-portable mode, additional config files such as TLS config are saved
// in 'Program Data' and are not stored in the same place as the settings file.
#ifdef Q_OS_WIN
  inline static const QString m_settingsPath = Settings::SystemDir;
#else
  inline static const QString m_settingsPath = m_settingsPathTemp;
#endif

  inline static const QString m_expectedTlsDir = QStringLiteral("%1/%2").arg(m_settingsPath, kTlsDirName);
  inline static const QString m_expectedTlsLocalDB =
      QStringLiteral("%1/%2").arg(m_expectedTlsDir, kTlsFingerprintLocalFilename);
  inline static const QString m_expectedTlsServerDB =
      QStringLiteral("%1/%2").arg(m_expectedTlsDir, kTlsFingerprintTrustedServersFilename);
  inline static const QString m_expectedTlsClientDB =
      QStringLiteral("%1/%2").arg(m_expectedTlsDir, kTlsFingerprintTrustedClientsFilename);
};

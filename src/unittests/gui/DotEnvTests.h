/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class DotEnvTests : public QObject
{
  Q_OBJECT
private slots:
  // Test are run in order top to bottom
  void initTestCase();
  void invalidFile();
  void validFile();

private:
  inline static const QString m_envFile = QStringLiteral("tmp/test/.env");
};

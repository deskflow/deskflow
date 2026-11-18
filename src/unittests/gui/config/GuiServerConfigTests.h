/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class GuiServerConfigTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void initTestCase();
  void init();
  void recall_legacyGrid_geometryFromCell();
  void recall_legacyGrid_geometrySaved();
  void addClient_newClient_leftOfServer();
  void commit_addedClient_geometryInComputerSettings();
  void recall_committedLayout_sameComputers();

private:
  inline static const QString m_settingsPath = QStringLiteral("tmp/test");
  inline static const QString m_settingsFile = QStringLiteral("%1/GuiServerConfigTests.conf").arg(m_settingsPath);
  inline static const QString m_stateFile = QStringLiteral("%1/GuiServerConfigTests.state").arg(m_settingsPath);
};

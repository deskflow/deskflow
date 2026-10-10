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
  void migratesLegacyGrid();
  void addClientPlacesNextToServer();
  void geometryRoundTrip();
  void resizeKeepsNoOverlap();

private:
  inline static const QString m_settingsPath = QStringLiteral("tmp/test");
  inline static const QString m_settingsFile = QStringLiteral("%1/GuiServerConfigTests.conf").arg(m_settingsPath);
  inline static const QString m_stateFile = QStringLiteral("%1/GuiServerConfigTests.state").arg(m_settingsPath);
};

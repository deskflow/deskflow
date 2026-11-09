/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "RecentServersButtonTests.h"
#include "gui/widgets/RecentServersButton.h"
#include <QMenu>
#include <QSignalSpy>

namespace {
void connected(RecentServersButton &button, const QString &host, int port = 24800)
{
  button.beginAttempt(host, port, true);
  button.secureSocket(true);
  button.connectionStateChanged(deskflow::core::ConnectionState::Connected);
}
} // namespace

void RecentServersButtonTests::successfulConnection_enablesOneClickReconnect()
{
  RecentServersButton button;
  QSignalSpy saved(&button, &RecentServersButton::historyChanged);
  QSignalSpy requested(&button, &RecentServersButton::reconnectRequested);
  button.setConnectionAllowed(true);
  QVERIFY(!button.isEnabled());
  connected(button, QStringLiteral("server.invalid"), 24801);
  QCOMPARE(saved.size(), 1);
  QCOMPARE(requested.size(), 0);
  QVERIFY(button.isEnabled());
  button.click();
  QCOMPARE(requested.size(), 1);
  QCOMPARE(requested.at(0).at(0).toString(), QStringLiteral("server.invalid"));
  QCOMPARE(requested.at(0).at(1).toInt(), 24801);
  button.click(); // Disabled synchronously: do not request overlapping restarts.
  QCOMPARE(requested.size(), 1);
}

void RecentServersButtonTests::menuSelection_keepsTheSelectedPort()
{
  RecentServersButton button;
  button.setConnectionAllowed(true);
  connected(button, QStringLiteral("first.invalid"), 24801);
  connected(button, QStringLiteral("second.invalid"), 24802);
  QSignalSpy requested(&button, &RecentServersButton::reconnectRequested);
  button.menu()->actions().at(1)->trigger();
  QCOMPARE(requested.size(), 1);
  QCOMPARE(requested.at(0).at(0).toString(), QStringLiteral("first.invalid"));
  QCOMPARE(requested.at(0).at(1).toInt(), 24801);
}

void RecentServersButtonTests::securityDisabled_doesNotRecordOrReconnect()
{
  RecentServersButton button;
  QSignalSpy saved(&button, &RecentServersButton::historyChanged);
  QSignalSpy requested(&button, &RecentServersButton::reconnectRequested);
  button.beginAttempt(QStringLiteral("unverified.invalid"), 24800, false);
  button.secureSocket(true);
  button.connectionStateChanged(deskflow::core::ConnectionState::Connected);
  QCOMPARE(saved.size(), 0);
  connected(button, QStringLiteral("verified.invalid"));
  button.setConnectionAllowed(false, QStringLiteral("TLS checks required"));
  button.menu()->actions().first()->trigger();
  button.click();
  QCOMPARE(requested.size(), 0);
}

void RecentServersButtonTests::cancelledAttempt_ignoresLateMessages()
{
  RecentServersButton button;
  QSignalSpy saved(&button, &RecentServersButton::historyChanged);
  button.beginAttempt(QStringLiteral("old.invalid"), 24800, true);
  button.secureSocket(true);
  button.cancelAttempt();
  button.connectionStateChanged(deskflow::core::ConnectionState::Connected);
  button.secureSocket(true);
  QCOMPARE(saved.size(), 0);
}

void RecentServersButtonTests::history_roundTripsWithoutStartingAConnection()
{
  RecentServersButton original;
  connected(original, QStringLiteral("[fe80::123%en1]"), 24801);
  RecentServersButton restored;
  QSignalSpy requested(&restored, &RecentServersButton::reconnectRequested);
  QSignalSpy saved(&restored, &RecentServersButton::historyChanged);
  restored.loadHistory(original.history());
  QCOMPARE(restored.history(), original.history());
  QCOMPARE(requested.size(), 0);
  QCOMPARE(saved.size(), 0);
  QVERIFY(!restored.isEnabled());
}

void RecentServersButtonTests::clearHistory_doesNotChangeTrustOrConnect()
{
  RecentServersButton button;
  button.setConnectionAllowed(true);
  connected(button, QStringLiteral("server.invalid"));
  QSignalSpy saved(&button, &RecentServersButton::historyChanged);
  QSignalSpy requested(&button, &RecentServersButton::reconnectRequested);
  button.menu()->actions().last()->trigger();
  QCOMPARE(saved.size(), 1);
  QCOMPARE(button.history(), QStringLiteral("[]"));
  QCOMPARE(requested.size(), 0);
  button.connectionStateChanged(deskflow::core::ConnectionState::Connected);
  button.secureSocket(true);
  QCOMPARE(saved.size(), 1);
}

void RecentServersButtonTests::malformedHistory_staysDisabled()
{
  RecentServersButton button;
  button.setConnectionAllowed(true);
  button.loadHistory(QStringLiteral(R"([{"address":"bad\nname","port":24800},{"address":"x","port":0}])"));
  QCOMPARE(button.history(), QStringLiteral("[]"));
  QVERIFY(!button.isEnabled());
  button.loadHistory(QString(8193, QLatin1Char('x')));
  QVERIFY(!button.isEnabled());
}

void RecentServersButtonTests::longTarget_isBoundedAndKeyboardAccessible()
{
  RecentServersButton button;
  const auto address = QString(220, QLatin1Char('a')) + QStringLiteral(".invalid");
  button.setConnectionAllowed(true);
  connected(button, address, 24801);
  QVERIFY(button.sizeHint().width() < 360);
  QVERIFY(button.accessibleName().contains(address));
  QVERIFY(button.accessibleName().contains(QStringLiteral("24801")));
  QVERIFY(button.toolTip().contains(address));
  QSignalSpy requested(&button, &RecentServersButton::reconnectRequested);
  button.show();
  QVERIFY(button.height() >= 32);
  QVERIFY(button.height() >= button.sizeHint().height());
  button.setFocus(Qt::TabFocusReason);
  QTest::keyClick(&button, Qt::Key_Space);
  QCOMPARE(requested.size(), 1);
  QCOMPARE(requested.at(0).at(0).toString(), address);
  QCOMPARE(requested.at(0).at(1).toInt(), 24801);
}

QTEST_MAIN(RecentServersButtonTests)

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/Clipboard.h"
#include "platform/PortalClipboard.h"

#include <QList>
#include <QTest>

class PortalClipboardTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void shouldPublish_sameClipboardTwice_secondCallReturnsFalse();
  void shouldPublish_changedClipboard_secondCallReturnsTrue();
  void reset_afterDuplicate_nextCallReturnsTrue();
};

void PortalClipboardTests::shouldPublish_sameClipboardTwice_secondCallReturnsFalse()
{
  Clipboard clipboard;
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::Text, "same payload");
  clipboard.close();
  deskflow::PortalClipboardClaimTracker tracker;

  const QList<bool> actual{tracker.shouldPublish(&clipboard), tracker.shouldPublish(&clipboard)};

  QCOMPARE(actual, QList<bool>({true, false}));
}

void PortalClipboardTests::shouldPublish_changedClipboard_secondCallReturnsTrue()
{
  Clipboard clipboard;
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::Text, "first payload");
  clipboard.close();
  deskflow::PortalClipboardClaimTracker tracker;
  QVERIFY(tracker.shouldPublish(&clipboard));

  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::Text, "second payload");
  clipboard.close();

  QVERIFY(tracker.shouldPublish(&clipboard));
}

void PortalClipboardTests::reset_afterDuplicate_nextCallReturnsTrue()
{
  Clipboard clipboard;
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::Text, "same payload");
  clipboard.close();
  deskflow::PortalClipboardClaimTracker tracker;
  QVERIFY(tracker.shouldPublish(&clipboard));
  QVERIFY(!tracker.shouldPublish(&clipboard));

  tracker.reset();

  QVERIFY(tracker.shouldPublish(&clipboard));
}

QTEST_MAIN(PortalClipboardTests)

#include "PortalClipboardTests.moc"

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class ISecondaryComputerTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void scaleBelowOneKeepsSlowScroll();
  void subNotchDeltasAddUpToNotches();
  void negativeSubNotchDeltasAddUpToNotches();
  void wholeNotchPassesStraightThrough();
  void reversingDirectionDropsHeldRemainder();

private:
  inline static const QString m_settingsFile = QStringLiteral("tmp/test/ISecondaryComputerTests.conf");
};

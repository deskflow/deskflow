/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ISecondaryComputerTests.h"

#include "deskflow/ISecondaryComputer.h"

#include <QFile>
#include <QScopeGuard>

namespace {
class TestComputer : public ISecondaryComputer
{
public:
  void fakeMouseButton(ButtonID, bool) override
  {
  }
  void fakeMouseMove(int32_t, int32_t) override
  {
  }
  void fakeMouseRelativeMove(int32_t, int32_t) const override
  {
  }
  void fakeMouseWheel(ScrollDelta) const override
  {
  }
};
} // namespace

void ISecondaryComputerTests::initTestCase()
{
  QFile::remove(m_settingsFile);
  Settings::setSettingsFile(m_settingsFile);
}

void ISecondaryComputerTests::scaleBelowOneKeepsSlowScroll()
{
  Settings::setValue(Settings::Client::YScrollScale, 0.5);
  const auto restoreScale = qScopeGuard([] { Settings::setValue(Settings::Client::YScrollScale, 1.0); });
  const TestComputer computer;

  // smooth scroll sends tiny deltas; truncating each 1 * 0.5 to 0 would drop them all
  const auto first = computer.applyScrollModifier({0, 1});
  const auto second = computer.applyScrollModifier({0, 1});

  QCOMPARE(first.y + second.y, 1);
}

void ISecondaryComputerTests::subNotchDeltasAddUpToNotches()
{
  const TestComputer computer;

  QCOMPARE(computer.takeWholeScrollNotches({0, 60}).y, 0);
  QCOMPARE(computer.takeWholeScrollNotches({0, 60}).y, 120);
  QCOMPARE(computer.takeWholeScrollNotches({0, 60}).y, 0);
}

void ISecondaryComputerTests::negativeSubNotchDeltasAddUpToNotches()
{
  const TestComputer computer;

  QCOMPARE(computer.takeWholeScrollNotches({-50, 0}).x, 0);
  QCOMPARE(computer.takeWholeScrollNotches({-50, 0}).x, 0);
  QCOMPARE(computer.takeWholeScrollNotches({-50, 0}).x, -120);
}

void ISecondaryComputerTests::wholeNotchPassesStraightThrough()
{
  const TestComputer computer;

  const auto delta = computer.takeWholeScrollNotches({120, -240});

  QCOMPARE(delta.x, 120);
  QCOMPARE(delta.y, -240);
}

void ISecondaryComputerTests::reversingDirectionDropsHeldRemainder()
{
  const TestComputer computer;

  QCOMPARE(computer.takeWholeScrollNotches({0, 100}).y, 0);
  // scrolling back must not first unwind the 100 still held from the other direction
  QCOMPARE(computer.takeWholeScrollNotches({0, -30}).y, 0);
  QCOMPARE(computer.takeWholeScrollNotches({0, -90}).y, -120);
}

QTEST_MAIN(ISecondaryComputerTests)

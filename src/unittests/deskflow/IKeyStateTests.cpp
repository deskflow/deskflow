/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IKeyStateTests.h"

#include "deskflow/IKeyState.h"

void IKeyStateTests::allocDestination()
{
  auto info = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1", "test2"});
  QCOMPARE(info->m_computers, ":test1:test2:");
}

void IKeyStateTests::alloc_noActiveComputerOnly_false()
{
  auto info = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1"});

  QVERIFY(!info->m_activeComputerOnly);
}

void IKeyStateTests::alloc_activeComputerOnly_true()
{
  auto info = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1"}, true);

  QVERIFY(info->m_activeComputerOnly);
}

void IKeyStateTests::allocCopy_activeComputerOnly_true()
{
  auto info = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1"}, true);

  auto copy = IKeyState::KeyInfo::alloc(*info);

  QVERIFY(copy->m_activeComputerOnly);
}

void IKeyStateTests::equal_activeComputerOnlyDiffers_false()
{
  auto a = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1"}, true);
  auto b = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1"});

  QVERIFY(!IKeyState::KeyInfo::equal(a, b));
}

QTEST_MAIN(IKeyStateTests)

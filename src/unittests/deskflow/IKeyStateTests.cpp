/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IKeyStateTests.h"

#include "../../lib/deskflow/IKeyState.h"

void IKeyStateTests::allocDestination()
{
  auto info = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1", "test2"});
  QCOMPARE(info->m_screensBuffer, ":test1:test2:");
}

QTEST_MAIN(IKeyStateTests)

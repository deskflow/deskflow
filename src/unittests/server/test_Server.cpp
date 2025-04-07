/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "test_Server.h"

void Server_Test::SwitchToScreenInfo_alloc_screen()
{
  auto actual = Server::SwitchToScreenInfo::alloc("test");
  QCOMPARE(actual->m_screen, "test");
}

void Server_Test::KeyboardBroadcastInfo_alloc_stateAndSceens()
{
  auto info = Server::KeyboardBroadcastInfo::alloc(Server::KeyboardBroadcastInfo::State::kOn, "test");
  QCOMPARE(info->m_state, Server::KeyboardBroadcastInfo::State::kOn);
  QCOMPARE(info->m_screens, "test");
}

QTEST_MAIN(Server_Test)

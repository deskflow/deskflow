/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerTests.h"

#include "server/Server.h"

void ServerTests::SwitchToScreenInfo_alloc_screen()
{
  auto actual = new Server::SwitchToScreenInfo("test");
  QCOMPARE(actual->m_screen, "test");
  delete actual;
}

void ServerTests::KeyboardBroadcastInfo_alloc_stateAndSceens()
{
  auto info = new Server::KeyboardBroadcastInfo(Server::KeyboardBroadcastInfo::State::kOn, "test");
  QCOMPARE(info->m_state, Server::KeyboardBroadcastInfo::State::kOn);
  QCOMPARE(info->m_screens, "test");
  delete info;
}

QTEST_MAIN(ServerTests)

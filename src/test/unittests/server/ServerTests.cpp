/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "lib/server/Server.h"

#include <gtest/gtest.h>

TEST(ServerTests, SwitchToScreenInfo_alloc_screen)
{
  auto info = Server::SwitchToScreenInfo::alloc("test");

  EXPECT_STREQ(info->m_screen, "test");
}

TEST(ServerTests, KeyboardBroadcastInfo_alloc_stateAndSceens)
{
  auto info = Server::KeyboardBroadcastInfo::alloc(Server::KeyboardBroadcastInfo::State::kOn, "test");

  EXPECT_EQ(info->m_state, Server::KeyboardBroadcastInfo::State::kOn);
  EXPECT_STREQ(info->m_screens, "test");
}

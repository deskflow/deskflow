/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#define TEST_ENV

#include "ipc/IpcSettingMessage.h"

#include <gtest/gtest.h>

TEST(IpcSettingMessage, testIpcSettingMessage)
{
  const std::string expected_name = "test";
  const std::string expected_value = "test_value";

  IpcSettingMessage message("test", "test_value");

  EXPECT_EQ(expected_name, message.getName());
  EXPECT_EQ(expected_value, message.getValue());
}

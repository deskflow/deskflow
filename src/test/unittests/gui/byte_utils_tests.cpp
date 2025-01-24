/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/byte_utils.h"

#include <gtest/gtest.h>

using namespace deskflow::gui;

TEST(byte_utils_tests, bytesToInt_size4)
{
  char buffer[4] = {0x01, 0x02, 0x03, 0x04};

  const auto i = bytesToInt(buffer, 4);

  EXPECT_EQ(i, 0x01020304);
}

TEST(byte_utils_tests, intToBytes_size4)
{
  QByteArray bytes = intToBytes(0x01020304);

  EXPECT_EQ(bytes.size(), 4);
  EXPECT_EQ(bytes[0], 0x01);
  EXPECT_EQ(bytes[1], 0x02);
  EXPECT_EQ(bytes[2], 0x03);
  EXPECT_EQ(bytes[3], 0x04);
}

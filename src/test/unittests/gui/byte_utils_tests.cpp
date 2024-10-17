/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

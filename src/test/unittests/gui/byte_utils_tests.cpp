/*
 * synergy -- mouse and keyboard sharing utility
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

using namespace synergy::gui;

TEST(byte_utils_tests, bytesToInt_size1) {
  char buffer[1] = {0x01};
  EXPECT_EQ(bytesToInt(buffer, 1), 1);
}

TEST(byte_utils_tests, bytesToInt_size2) {
  char buffer[2] = {0x01, 0x02};
  EXPECT_EQ(bytesToInt(buffer, 2), 0x0102);
}

TEST(byte_utils_tests, bytesToInt_size4) {
  char buffer[4] = {0x01, 0x02, 0x03, 0x04};
  EXPECT_EQ(bytesToInt(buffer, 4), 0x01020304);
}

TEST(byte_utils_tests, intToBytes_size1) {
  char buffer[1] = {0};
  intToBytes(1, buffer, 1);
  EXPECT_EQ(buffer[0], 1);
}

TEST(byte_utils_tests, intToBytes_size2) {
  char buffer[2] = {0};
  intToBytes(0x0102, buffer, 2);
  EXPECT_EQ(buffer[0], 0x01);
  EXPECT_EQ(buffer[1], 0x02);
}

TEST(byte_utils_tests, intToBytes_size4) {
  char buffer[4] = {0};
  intToBytes(0x01020304, buffer, 4);
  EXPECT_EQ(buffer[0], 0x01);
  EXPECT_EQ(buffer[1], 0x02);
  EXPECT_EQ(buffer[2], 0x03);
  EXPECT_EQ(buffer[3], 0x04);
}

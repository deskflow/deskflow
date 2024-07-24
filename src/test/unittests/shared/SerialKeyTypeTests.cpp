/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#define TEST_ENV

#include "license/SerialKeyType.h"

#include <gtest/gtest.h>

TEST(SerialKeyTypeTests, TrialTemporaryKeyType_false) {
  SerialKeyType KeyType;
  EXPECT_EQ(false, KeyType.isTrial());
  EXPECT_EQ(false, KeyType.isTimeLimited());
}

TEST(SerialKeyTypeTests, TrialTemporaryKeyType_true) {
  SerialKeyType KeyType;
  KeyType.setType("trial");
  EXPECT_EQ(true, KeyType.isTrial());
  EXPECT_EQ(true, KeyType.isTimeLimited());
}

TEST(SerialKeyTypeTests, TimeLimitedKeyType_true) {
  SerialKeyType KeyType;
  KeyType.setType("subscription");
  EXPECT_EQ(false, KeyType.isTrial());
  EXPECT_EQ(true, KeyType.isTimeLimited());
}

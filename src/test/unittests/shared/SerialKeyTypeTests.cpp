/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Inc.
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

#include "shared/SerialKeyType.h"

#include "test/global/gtest.h"

TEST(SerialKeyTypeTests, TrialTemporaryKeyType_false)
{
	SerialKeyType KeyType;
	EXPECT_EQ(false, KeyType.isTrial());
	EXPECT_EQ(false, KeyType.isTemporary());
	EXPECT_FALSE(KeyType.isMaintenance());
}

TEST(SerialKeyTypeTests, TrialTemporaryKeyType_true)
{
	SerialKeyType KeyType;
	KeyType.setKeyType("trial");
	EXPECT_EQ(true, KeyType.isTrial());
	EXPECT_EQ(true, KeyType.isTemporary());
	EXPECT_FALSE(KeyType.isMaintenance());
}

TEST(SerialKeyTypeTests, TemporaryKeyType_true)
{
	SerialKeyType KeyType;
	KeyType.setKeyType("subscription");
	EXPECT_EQ(false, KeyType.isTrial());
	EXPECT_EQ(true, KeyType.isTemporary());
	EXPECT_FALSE(KeyType.isMaintenance());
}

TEST(SerialKeyTypeTests, MaintanenceKeyType_true)
{
	SerialKeyType KeyType;
	KeyType.setKeyType("maintenance");
	EXPECT_EQ(false, KeyType.isTrial());
	EXPECT_EQ(false, KeyType.isTemporary());
	EXPECT_TRUE(KeyType.isMaintenance());
}

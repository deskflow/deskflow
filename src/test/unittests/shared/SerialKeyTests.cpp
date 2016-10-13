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

#include "shared/SerialKey.h"

#include "test/global/gtest.h"

TEST(SerialKeyTests, decode_empty_returnEmptyString)
{
	SerialKey serial("");
	std::string plainText = serial.decode("");
	EXPECT_EQ(0, plainText.size());
}

TEST(SerialKeyTests, decode_invalidDigit_returnEmptyString)
{
	SerialKey serial("");
	std::string plainText = serial.decode("MOCKZ");
	EXPECT_EQ(0, plainText.size());
}

TEST(SerialKeyTests, decode_validSerial_returnPlainText)
{
	SerialKey serial("");
	std::string plainText = serial.decode("53796E6572677920726F636B7321");
	EXPECT_EQ("Synergy rocks!", plainText);
}

TEST(SerialKeyTests, parse_noParty_invalid)
{
	SerialKey serial("");
	serial.parse("MOCK");
	EXPECT_FALSE(serial.isValid(0));
}

TEST(SerialKeyTests, parse_invalidPartsLenghth_invalid)
{
	SerialKey serial("");
	serial.parse("{Synergy;Rocks}");
	EXPECT_FALSE(serial.isValid(0));
}

TEST(SerialKeyTests, parse_validV1Serial_valid)
{
	SerialKey serial("");
	serial.parse("{v1;basic;Bob;1;email;company name;0;86400}");
	EXPECT_EQ(true, serial.isValid(0));
	EXPECT_EQ(kBasic, serial.edition());
	EXPECT_FALSE(serial.isExpired(0));
	EXPECT_EQ(true, serial.dayLeft(0));
	EXPECT_EQ(true, serial.isExpiring(1));
}

TEST(SerialKeyTests, parse_validV2Serial_valid)
{
	SerialKey serial("");
	serial.parse("{v2;trial;pro;Bob;1;email;company name;0;86400}");
	EXPECT_EQ(true, serial.isValid(0));
	EXPECT_EQ(kPro, serial.edition());
	EXPECT_FALSE(serial.isExpired(0));
	EXPECT_EQ(true, serial.dayLeft(0));
	EXPECT_EQ(true, serial.isExpiring(1));
	EXPECT_EQ(true, serial.isTrial());
}

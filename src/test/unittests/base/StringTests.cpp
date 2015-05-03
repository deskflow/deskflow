/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
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

#include "base/String.h"

#include "test/global/gtest.h"

using namespace synergy;

TEST(StringTests, format)
{
	const char* format = "%%%{1}=%{2}";
	const char* arg1 = "answer";
	const char* arg2 = "42";

	String result = string::format(format, arg1, arg2);

	EXPECT_EQ("%answer=42", result);
}

TEST(StringTests, findReplaceAll)
{
	String subject = "foobar";
	String find = "bar";
	String replace = "baz";

	string::findReplaceAll(subject, find, replace);

	EXPECT_EQ("foobaz", subject);
}

TEST(StringTests, sprintf)
{
	const char* format = "%s=%d";
	const char* arg1 = "answer";
	int arg2 = 42;

	String result = string::sprintf(format, arg1, arg2);

	EXPECT_EQ("answer=42", result);
}

TEST(StringTests, toHex)
{
	String subject = "foobar";
	int width = 2;

	string::toHex(subject, width);

	EXPECT_EQ("666f6f626172", subject);
}

TEST(StringTests, uppercase)
{
	String subject = "12foo3BaR";

	string::uppercase(subject);

	EXPECT_EQ("12FOO3BAR", subject);
}

TEST(StringTests, removeChar)
{
	String subject = "foobar";
	const char c = 'o';

	string::removeChar(subject, c);

	EXPECT_EQ("fbar", subject);
}

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

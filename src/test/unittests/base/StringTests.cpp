/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
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

TEST(CStringTests, find_replace_all)
{
	CString subject = "foobar";
	CString find = "bar";
	CString replace = "baz";

	find_replace_all(subject, find, replace);

	EXPECT_EQ("foobaz", subject);
}

TEST(CStringTests, string_format)
{
	CString format = "%s=%d";
	const char* arg1 = "answer";
	int arg2 = 42;

	CString result = string_format(format, arg1, arg2);

	EXPECT_EQ("answer=42", result);
}

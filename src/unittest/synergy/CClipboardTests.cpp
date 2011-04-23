/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2010 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include <gtest/gtest.h>
#include "CClipboard.h"

TEST(CClipboardTests, empty_wasOpened_returnsTrue)
{
	CClipboard clipboard;
	clipboard.open(true);

	bool actual = clipboard.empty();

	EXPECT_EQ(true, actual);
}

TEST(CClipboardTests, add_newValue_valueWasStored)
{
	CClipboard clipboard;
	clipboard.open(true);
	IClipboard::EFormat format(IClipboard::kText);

	clipboard.add(format, "Hello world!");

	CString actual = clipboard.get(format);
	EXPECT_EQ("Hello world!", actual);
}

TEST(CClipboardTests, add_replaceValue_valueWasStored)
{
	CClipboard clipboard;
	clipboard.open(true);
	IClipboard::EFormat format(IClipboard::kText);

	clipboard.add(format, "Hello world!");
	clipboard.add(format, "Goodbye world!");

	CString actual = clipboard.get(format);
	EXPECT_EQ("Goodbye world!", actual);
}

TEST(CClipboardTests, open_defaultState_returnsTrue)
{
	CClipboard clipboard;

	bool actual = clipboard.open(true);

	EXPECT_EQ(true, actual);
}

TEST(CClipboardTests, getTime_defaultState_returnsZero)
{
	CClipboard clipboard;

	CClipboard::Time actual = clipboard.getTime();

	CClipboard::Time expected(0);
	EXPECT_EQ(expected, actual);
}

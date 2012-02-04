/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include <iostream>
#include <gtest/gtest.h>
#include "COSXClipboard.h"

TEST(COSXClipboardTests, empty_openCalled_returnsTrue)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	
	bool actual = clipboard.empty();
	
	EXPECT_EQ(true, actual);
}

TEST(COSXClipboardTests, empty_singleFormat_hasReturnsFalse)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	clipboard.add(COSXClipboard::kText, "synergy rocks!");
	
	clipboard.empty();
	
	bool actual = clipboard.has(COSXClipboard::kText);
	EXPECT_EQ(false, actual);
}

TEST(COSXClipboardTests, add_newValue_valueWasStored)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	
	clipboard.add(IClipboard::kText, "synergy rocks!");
	
	CString actual = clipboard.get(IClipboard::kText);
	EXPECT_EQ("synergy rocks!", actual);
}

TEST(COSXClipboardTests, add_replaceValue_valueWasReplaced)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	
	clipboard.add(IClipboard::kText, "synergy rocks!");
	clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.
	
	CString actual = clipboard.get(IClipboard::kText);
	EXPECT_EQ("maxivista sucks", actual);
}

TEST(COSXClipboardTests, open_timeIsZero_returnsTrue)
{
	COSXClipboard clipboard;
	
	bool actual = clipboard.open(0);
	
	EXPECT_EQ(true, actual);
}

TEST(COSXClipboardTests, open_timeIsOne_returnsTrue)
{
	COSXClipboard clipboard;
	
	bool actual = clipboard.open(1);
	
	EXPECT_EQ(true, actual);
}

TEST(COSXClipboardTests, close_isOpen_noErrors)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	
	clipboard.close();
	
	// can't assert anything
}

TEST(COSXClipboardTests, getTime_openWithNoEmpty_returnsOne)
{
	COSXClipboard clipboard;
	clipboard.open(1);
	
	COSXClipboard::Time actual = clipboard.getTime();
	
	// this behavior is different to that of CClipboard which only
	// returns the value passed into open(t) after empty() is called.
	EXPECT_EQ((UInt32)1, actual);
}

TEST(COSXClipboardTests, getTime_openAndEmpty_returnsOne)
{
	COSXClipboard clipboard;
	clipboard.open(1);
	clipboard.empty();
	
	COSXClipboard::Time actual = clipboard.getTime();
	
	EXPECT_EQ((UInt32)1, actual);
}

TEST(COSXClipboardTests, has_withFormatAdded_returnsTrue)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	clipboard.empty();
	clipboard.add(IClipboard::kText, "synergy rocks!");
	
	bool actual = clipboard.has(IClipboard::kText);
	
	EXPECT_EQ(true, actual);
}

TEST(COSXClipboardTests, has_withNoFormats_returnsFalse)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	clipboard.empty();
	
	bool actual = clipboard.has(IClipboard::kText);
	
	EXPECT_EQ(false, actual);
}

TEST(COSXClipboardTests, get_withNoFormats_returnsEmpty)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	clipboard.empty();
	
	CString actual = clipboard.get(IClipboard::kText);
	
	EXPECT_EQ("", actual);
}

TEST(COSXClipboardTests, get_withFormatAdded_returnsExpected)
{
	COSXClipboard clipboard;
	clipboard.open(0);
	clipboard.empty();
	clipboard.add(IClipboard::kText, "synergy rocks!");
	
	CString actual = clipboard.get(IClipboard::kText);
	
	EXPECT_EQ("synergy rocks!", actual);
}

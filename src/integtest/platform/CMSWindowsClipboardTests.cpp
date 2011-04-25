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

#include <gtest/gtest.h>
#include "CMSWindowsClipboard.h"

TEST(CMSWindowsClipboardTests, emptyUnowned_openCalled_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	bool actual = clipboard.emptyUnowned();

	EXPECT_EQ(true, actual);
}

TEST(CMSWindowsClipboardTests, empty_openCalled_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	bool actual = clipboard.empty();

	EXPECT_EQ(true, actual);
}

TEST(CMSWindowsClipboardTests, empty_singleFormat_hasReturnsFalse)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.add(CMSWindowsClipboard::kText, "synergy rocks!");

	clipboard.empty();

	bool actual = clipboard.has(CMSWindowsClipboard::kText);
	EXPECT_EQ(false, actual);
}

TEST(CMSWindowsClipboardTests, add_newValue_valueWasStored)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	clipboard.add(IClipboard::kText, "synergy rocks!");

	CString actual = clipboard.get(IClipboard::kText);
	EXPECT_EQ("synergy rocks!", actual);
}

TEST(CMSWindowsClipboardTests, add_replaceValue_valueWasReplaced)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	clipboard.add(IClipboard::kText, "synergy rocks!");
	clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.

	CString actual = clipboard.get(IClipboard::kText);
	EXPECT_EQ("maxivista sucks", actual);
}

TEST(CMSWindowsClipboardTests, open_timeIsZero_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);

	bool actual = clipboard.open(0);

	EXPECT_EQ(true, actual);
}

TEST(CMSWindowsClipboardTests, open_timeIsOne_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);

	bool actual = clipboard.open(1);

	EXPECT_EQ(true, actual);
}

TEST(CMSWindowsClipboardTests, close_isOpen_noErrors)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	clipboard.close();

	// can't assert anything
}

// looks like this test may fail intermittently:
// * http://buildbot.synergy-foss.org:8000/builders/trunk-win32/builds/246/steps/shell_3/logs/stdio
TEST(CMSWindowsClipboardTests, getTime_openWithNoEmpty_returnsOne)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(1);

	CMSWindowsClipboard::Time actual = clipboard.getTime();

	// this behavior is different to that of CClipboard which only
	// returns the value passed into open(t) after empty() is called.
	EXPECT_EQ(1, actual);
}

TEST(CMSWindowsClipboardTests, getTime_openAndEmpty_returnsOne)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(1);
	clipboard.empty();

	CMSWindowsClipboard::Time actual = clipboard.getTime();

	EXPECT_EQ(1, actual);
}

TEST(CMSWindowsClipboardTests, has_withFormatAdded_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.empty();
	clipboard.add(IClipboard::kText, "synergy rocks!");

	bool actual = clipboard.has(IClipboard::kText);

	EXPECT_EQ(true, actual);
}

TEST(CMSWindowsClipboardTests, has_withNoFormats_returnsFalse)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.empty();

	bool actual = clipboard.has(IClipboard::kText);

	EXPECT_EQ(false, actual);
}

TEST(CMSWindowsClipboardTests, get_withNoFormats_returnsEmpty)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.empty();

	CString actual = clipboard.get(IClipboard::kText);

	EXPECT_EQ("", actual);
}

TEST(CMSWindowsClipboardTests, get_withFormatAdded_returnsExpected)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.empty();
	clipboard.add(IClipboard::kText, "synergy rocks!");

	CString actual = clipboard.get(IClipboard::kText);

	EXPECT_EQ("synergy rocks!", actual);
}

TEST(CMSWindowsClipboardTests, isOwnedBySynergy_defaultState_noError)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	bool actual = clipboard.isOwnedBySynergy();

	EXPECT_EQ(true, actual);
}

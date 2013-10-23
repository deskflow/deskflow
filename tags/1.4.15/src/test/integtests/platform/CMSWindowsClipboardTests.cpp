/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Nick Bolton
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
#include <gmock/gmock.h>
#include "CMSWindowsClipboard.h"
#include "IMSWindowsClipboardFacade.h"

class CMSWindowsClipboardTests : public ::testing::Test
{
protected:
	virtual void SetUp()
	{
		emptyClipboard();
	}

	virtual void TearDown()
	{
		emptyClipboard();
	}

private:
	void emptyClipboard() 
	{
		CMSWindowsClipboard clipboard(NULL);
		clipboard.open(0);
		clipboard.empty();
	}
};

class MockFacade : public IMSWindowsClipboardFacade
{
public:
	MOCK_METHOD2(write, void(HANDLE, UINT));
};

TEST_F(CMSWindowsClipboardTests, emptyUnowned_openCalled_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	bool actual = clipboard.emptyUnowned();

	EXPECT_EQ(true, actual);
}

TEST_F(CMSWindowsClipboardTests, empty_openCalled_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	bool actual = clipboard.empty();

	EXPECT_EQ(true, actual);
}

TEST_F(CMSWindowsClipboardTests, empty_singleFormat_hasReturnsFalse)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.add(CMSWindowsClipboard::kText, "synergy rocks!");

	clipboard.empty();

	bool actual = clipboard.has(CMSWindowsClipboard::kText);
	EXPECT_EQ(false, actual);
}

TEST_F(CMSWindowsClipboardTests, add_newValue_valueWasStored)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	clipboard.add(IClipboard::kText, "synergy rocks!");

	CString actual = clipboard.get(IClipboard::kText);
	EXPECT_EQ("synergy rocks!", actual);
}

TEST_F(CMSWindowsClipboardTests, add_newValue_writeWasCalled)
{
	MockFacade facade;
	EXPECT_CALL(facade, write(testing::_, testing::_));

	CMSWindowsClipboard clipboard(NULL);
	clipboard.setFacade(facade);
	clipboard.open(0);

	clipboard.add(IClipboard::kText, "synergy rocks!");
}

TEST_F(CMSWindowsClipboardTests, add_replaceValue_valueWasReplaced)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	clipboard.add(IClipboard::kText, "synergy rocks!");
	clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.

	CString actual = clipboard.get(IClipboard::kText);
	EXPECT_EQ("maxivista sucks", actual);
}

TEST_F(CMSWindowsClipboardTests, open_timeIsZero_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);

	bool actual = clipboard.open(0);

	EXPECT_EQ(true, actual);
}

TEST_F(CMSWindowsClipboardTests, open_timeIsOne_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);

	bool actual = clipboard.open(1);

	EXPECT_EQ(true, actual);
}

TEST_F(CMSWindowsClipboardTests, close_isOpen_noErrors)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	clipboard.close();

	// can't assert anything
}

// looks like this test may fail intermittently:
// * http://buildbot.synergy-foss.org:8000/builders/trunk-win32/builds/246/steps/shell_3/logs/stdio
/*TEST_F(CMSWindowsClipboardTests, getTime_openWithNoEmpty_returnsOne)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(1);

	CMSWindowsClipboard::Time actual = clipboard.getTime();

	// this behavior is different to that of CClipboard which only
	// returns the value passed into open(t) after empty() is called.
	EXPECT_EQ(1, actual);
}*/

// this also fails intermittently:
// http://buildbot.synergy-foss.org:8000/builders/trunk-win32/builds/266/steps/shell_3/logs/stdio
/*TEST_F(CMSWindowsClipboardTests, getTime_openAndEmpty_returnsOne)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(1);
	clipboard.empty();

	CMSWindowsClipboard::Time actual = clipboard.getTime();

	EXPECT_EQ(1, actual);
}*/

TEST_F(CMSWindowsClipboardTests, has_withFormatAdded_returnsTrue)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.empty();
	clipboard.add(IClipboard::kText, "synergy rocks!");

	bool actual = clipboard.has(IClipboard::kText);

	EXPECT_EQ(true, actual);
}

TEST_F(CMSWindowsClipboardTests, has_withNoFormats_returnsFalse)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.empty();

	bool actual = clipboard.has(IClipboard::kText);

	EXPECT_EQ(false, actual);
}

TEST_F(CMSWindowsClipboardTests, get_withNoFormats_returnsEmpty)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.empty();

	CString actual = clipboard.get(IClipboard::kText);

	EXPECT_EQ("", actual);
}

TEST_F(CMSWindowsClipboardTests, get_withFormatAdded_returnsExpected)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);
	clipboard.empty();
	clipboard.add(IClipboard::kText, "synergy rocks!");

	CString actual = clipboard.get(IClipboard::kText);

	EXPECT_EQ("synergy rocks!", actual);
}

TEST_F(CMSWindowsClipboardTests, isOwnedBySynergy_defaultState_noError)
{
	CMSWindowsClipboard clipboard(NULL);
	clipboard.open(0);

	bool actual = clipboard.isOwnedBySynergy();

	EXPECT_EQ(true, actual);
}

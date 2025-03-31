/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/IMSWindowsClipboardFacade.h"
#include "platform/MSWindowsClipboard.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MSWindowsClipboardTests : public ::testing::Test
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
    MSWindowsClipboard clipboard(NULL);
    clipboard.open(0);
    clipboard.empty();
  }
};

class MockFacade : public IMSWindowsClipboardFacade
{
public:
  MOCK_METHOD2(write, void(HANDLE, UINT));
};

TEST_F(MSWindowsClipboardTests, emptyUnowned_openCalled_returnsTrue)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);

  bool actual = clipboard.emptyUnowned();

  EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, empty_openCalled_returnsTrue)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);

  bool actual = clipboard.empty();

  EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, empty_singleFormat_hasReturnsFalse)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);
  clipboard.add(MSWindowsClipboard::kText, "synergy rocks!");

  clipboard.empty();

  bool actual = clipboard.has(MSWindowsClipboard::kText);
  EXPECT_EQ(false, actual);
}

TEST_F(MSWindowsClipboardTests, add_newValue_valueWasStored)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);

  clipboard.add(IClipboard::kText, "synergy rocks!");

  std::string actual = clipboard.get(IClipboard::kText);
  EXPECT_EQ("synergy rocks!", actual);
}

TEST_F(MSWindowsClipboardTests, add_newValue_writeWasCalled)
{
  MockFacade facade;
  EXPECT_CALL(facade, write(testing::_, testing::_));

  MSWindowsClipboard clipboard(NULL);
  clipboard.setFacade(facade);
  clipboard.open(0);

  clipboard.add(IClipboard::kText, "synergy rocks!");
}

TEST_F(MSWindowsClipboardTests, add_replaceValue_valueWasReplaced)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);

  clipboard.add(IClipboard::kText, "synergy rocks!");
  clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.

  std::string actual = clipboard.get(IClipboard::kText);
  EXPECT_EQ("maxivista sucks", actual);
}

TEST_F(MSWindowsClipboardTests, open_timeIsZero_returnsTrue)
{
  MSWindowsClipboard clipboard(NULL);

  bool actual = clipboard.open(0);

  EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, open_timeIsOne_returnsTrue)
{
  MSWindowsClipboard clipboard(NULL);

  bool actual = clipboard.open(1);

  EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, close_isOpen_noErrors)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);

  clipboard.close();

  // can't assert anything
}

// TODO: fix intermittently failing tests
#if 0
TEST_F(MSWindowsClipboardTests, getTime_openWithNoEmpty_returnsOne)
{
    MSWindowsClipboard clipboard(NULL);
    clipboard.open(1);

    MSWindowsClipboard::Time actual = clipboard.getTime();

    // this behavior is different to that of Clipboard which only
    // returns the value passed into open(t) after empty() is called.
    EXPECT_EQ(1, actual);
}

TEST_F(MSWindowsClipboardTests, getTime_openAndEmpty_returnsOne)
{
    MSWindowsClipboard clipboard(NULL);
    clipboard.open(1);
    clipboard.empty();

    MSWindowsClipboard::Time actual = clipboard.getTime();

    EXPECT_EQ(1, actual);
}
#endif

TEST_F(MSWindowsClipboardTests, has_withFormatAdded_returnsTrue)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);
  clipboard.empty();
  clipboard.add(IClipboard::kText, "synergy rocks!");

  bool actual = clipboard.has(IClipboard::kText);

  EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, has_withNoFormats_returnsFalse)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);
  clipboard.empty();

  bool actual = clipboard.has(IClipboard::kText);

  EXPECT_EQ(false, actual);
}

TEST_F(MSWindowsClipboardTests, get_withNoFormats_returnsEmpty)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);
  clipboard.empty();

  std::string actual = clipboard.get(IClipboard::kText);

  EXPECT_EQ("", actual);
}

TEST_F(MSWindowsClipboardTests, get_withFormatAdded_returnsExpected)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);
  clipboard.empty();
  clipboard.add(IClipboard::kText, "synergy rocks!");

  std::string actual = clipboard.get(IClipboard::kText);

  EXPECT_EQ("synergy rocks!", actual);
}

TEST_F(MSWindowsClipboardTests, isOwnedByDeskflow_defaultState_noError)
{
  MSWindowsClipboard clipboard(NULL);
  clipboard.open(0);

  bool actual = clipboard.isOwnedByDeskflow();

  EXPECT_EQ(true, actual);
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: fix failing tests (e.g. add_newValue_valueWasStored)
#if 0

#include "platform/OSXClipboard.h"

#include <gtest/gtest.h>

TEST(OSXClipboardTests, empty_openCalled_returnsTrue) {
  OSXClipboard clipboard;
  clipboard.open(0);

  bool actual = clipboard.empty();

  EXPECT_EQ(true, actual);
}

TEST(OSXClipboardTests, empty_singleFormat_hasReturnsFalse) {
  OSXClipboard clipboard;
  clipboard.open(0);
  clipboard.add(OSXClipboard::kText, "synergy rocks!");

  clipboard.empty();

  bool actual = clipboard.has(OSXClipboard::kText);
  EXPECT_EQ(false, actual);
}

TEST(OSXClipboardTests, add_newValue_valueWasStored) {
  OSXClipboard clipboard;
  clipboard.open(0);

  clipboard.add(IClipboard::kText, "synergy rocks!");

  std::string actual = clipboard.get(IClipboard::kText);
  EXPECT_EQ("synergy rocks!", actual);
}

TEST(OSXClipboardTests, add_replaceValue_valueWasReplaced) {
  OSXClipboard clipboard;
  clipboard.open(0);

  clipboard.add(IClipboard::kText, "synergy rocks!");
  clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.

  std::string actual = clipboard.get(IClipboard::kText);
  EXPECT_EQ("maxivista sucks", actual);
}

TEST(OSXClipboardTests, open_timeIsZero_returnsTrue) {
  OSXClipboard clipboard;

  bool actual = clipboard.open(0);

  EXPECT_EQ(true, actual);
}

TEST(OSXClipboardTests, open_timeIsOne_returnsTrue) {
  OSXClipboard clipboard;

  bool actual = clipboard.open(1);

  EXPECT_EQ(true, actual);
}

TEST(OSXClipboardTests, close_isOpen_noErrors) {
  OSXClipboard clipboard;
  clipboard.open(0);

  clipboard.close();

  // can't assert anything
}

TEST(OSXClipboardTests, getTime_openWithNoEmpty_returnsOne) {
  OSXClipboard clipboard;
  clipboard.open(1);

  OSXClipboard::Time actual = clipboard.getTime();

  // this behavior is different to that of Clipboard which only
  // returns the value passed into open(t) after empty() is called.
  EXPECT_EQ((uint32_t)1, actual);
}

TEST(OSXClipboardTests, getTime_openAndEmpty_returnsOne) {
  OSXClipboard clipboard;
  clipboard.open(1);
  clipboard.empty();

  OSXClipboard::Time actual = clipboard.getTime();

  EXPECT_EQ((uint32_t)1, actual);
}

TEST(OSXClipboardTests, has_withFormatAdded_returnsTrue) {
  OSXClipboard clipboard;
  clipboard.open(0);
  clipboard.empty();
  clipboard.add(IClipboard::kText, "synergy rocks!");

  bool actual = clipboard.has(IClipboard::kText);

  EXPECT_EQ(true, actual);
}

TEST(OSXClipboardTests, has_withNoFormats_returnsFalse) {
  OSXClipboard clipboard;
  clipboard.open(0);
  clipboard.empty();

  bool actual = clipboard.has(IClipboard::kText);

  EXPECT_EQ(false, actual);
}

TEST(OSXClipboardTests, get_withNoFormats_returnsEmpty) {
  OSXClipboard clipboard;
  clipboard.open(0);
  clipboard.empty();

  std::string actual = clipboard.get(IClipboard::kText);

  EXPECT_EQ("", actual);
}

TEST(OSXClipboardTests, get_withFormatAdded_returnsExpected) {
  OSXClipboard clipboard;
  clipboard.open(0);
  clipboard.empty();
  clipboard.add(IClipboard::kText, "synergy rocks!");

  std::string actual = clipboard.get(IClipboard::kText);

  EXPECT_EQ("synergy rocks!", actual);
}

#endif

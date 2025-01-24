/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboard.h"

#include "test/shared/undef_x11_macros.h"

#include <gtest/gtest.h>
#include <memory>

// TODO: fix segfault in release mode
#if 0

class XWindowsClipboardTests : public ::testing::Test {
protected:
  void SetUp() override {
    m_display = XOpenDisplay(nullptr);
    int screen = DefaultScreen(m_display);
    Window root = XRootWindow(m_display, screen);

    XSetWindowAttributes attr;
    attr.do_not_propagate_mask = 0;
    attr.override_redirect = True;
    attr.cursor = Cursor();

    m_window = XCreateWindow(
        m_display, root, 0, 0, 1, 1, 0, 0, InputOnly, nullptr, 0, &attr);

    m_clipboard = std::make_unique<XWindowsClipboard>(m_display, m_window, 0);
    m_clipboard->open(0);
    m_clipboard->empty();
  }

  void TearDown() override {
    XDestroyWindow(m_display, m_window);
    XCloseDisplay(m_display);
  }

  XWindowsClipboard &getClipboard() { return *m_clipboard; }

private:
  Display *m_display;
  Window m_window;
  std::unique_ptr<XWindowsClipboard> m_clipboard;
};

TEST_F(XWindowsClipboardTests, empty_openCalled_returnsTrue) {
  XWindowsClipboard &clipboard = getClipboard();

  bool actual = clipboard.empty();

  EXPECT_EQ(true, actual);
}

TEST_F(XWindowsClipboardTests, empty_singleFormat_hasReturnsFalse) {
  XWindowsClipboard &clipboard = getClipboard();
  clipboard.add(XWindowsClipboard::kText, "synergy rocks!");

  clipboard.empty();

  bool actual = clipboard.has(XWindowsClipboard::kText);
  EXPECT_FALSE(actual);
}

TEST_F(XWindowsClipboardTests, add_newValue_valueWasStored) {
  XWindowsClipboard &clipboard = getClipboard();

  clipboard.add(IClipboard::kText, "synergy rocks!");

  std::string actual = clipboard.get(IClipboard::kText);
  EXPECT_EQ("synergy rocks!", actual);
}

TEST_F(XWindowsClipboardTests, add_replaceValue_valueWasReplaced) {
  XWindowsClipboard &clipboard = getClipboard();

  clipboard.add(IClipboard::kText, "synergy rocks!");
  clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.

  std::string actual = clipboard.get(IClipboard::kText);
  EXPECT_EQ("maxivista sucks", actual);
}

TEST_F(XWindowsClipboardTests, close_isOpen_noErrors) {
  XWindowsClipboard &clipboard = getClipboard();

  // clipboard opened in createClipboard()
  clipboard.close();

  // can't assert anything
}

TEST_F(XWindowsClipboardTests, has_withFormatAdded_returnsTrue) {
  XWindowsClipboard &clipboard = getClipboard();
  clipboard.add(IClipboard::kText, "synergy rocks!");

  bool actual = clipboard.has(IClipboard::kText);

  EXPECT_EQ(true, actual);
}

TEST_F(XWindowsClipboardTests, has_withNoFormats_returnsFalse) {
  XWindowsClipboard &clipboard = getClipboard();

  bool actual = clipboard.has(IClipboard::kText);

  EXPECT_FALSE(actual);
}

TEST_F(XWindowsClipboardTests, get_withNoFormats_returnsEmpty) {
  XWindowsClipboard &clipboard = getClipboard();

  std::string actual = clipboard.get(IClipboard::kText);

  EXPECT_EQ("", actual);
}

TEST_F(XWindowsClipboardTests, get_withFormatAdded_returnsExpected) {
  XWindowsClipboard &clipboard = getClipboard();
  clipboard.add(IClipboard::kText, "synergy rocks!");

  std::string actual = clipboard.get(IClipboard::kText);

  EXPECT_EQ("synergy rocks!", actual);
}

#endif

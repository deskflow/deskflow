/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "XWindowsClipboardTests.h"

#include "../../lib/platform/XWindowsClipboard.h"

class TestXWindowsClipboard : public XWindowsClipboard
{
public:
  class TestCICCCMGetClipboard : public CICCCMGetClipboard
  {
  public:
    TestCICCCMGetClipboard() : CICCCMGetClipboard(None, None, None)
    {
    }
  };
};

void XWindowsClipboardTests::defaultCtor()
{
  TestXWindowsClipboard::TestCICCCMGetClipboard clipboard;
  QCOMPARE(None, clipboard.error());
}

// Only work on XWindows
#if !WINAPI_LIBEI && !WINAPI_PORTAL
void XWindowsClipboardTests::initTestCase()
{
  m_display = XOpenDisplay(nullptr);
  int screen = DefaultScreen(m_display);
  Window root = XRootWindow(m_display, screen);

  XSetWindowAttributes attr;
  attr.do_not_propagate_mask = 0;
  attr.override_redirect = True;
  attr.cursor = Cursor();

  m_window = XCreateWindow(m_display, root, 0, 0, 1, 1, 0, 0, InputOnly, nullptr, 0, &attr);
}

void XWindowsClipboardTests::cleanupTestCase()
{
  XDestroyWindow(m_display, m_window);
  XCloseDisplay(m_display);
}

void XWindowsClipboardTests::open()
{
  m_clipboard = std::make_unique<XWindowsClipboard>(m_display, m_window, 0);
  QVERIFY(m_clipboard->open(0));
  QVERIFY(m_clipboard->empty());
}

void XWindowsClipboardTests::singleFormat()
{
  auto &clipboard = getClipboard();
  QVERIFY(clipboard.empty());
  clipboard.add(XWindowsClipboard::kText, m_testString);
  QVERIFY(!clipboard.has(XWindowsClipboard::kText));
  QCOMPARE(clipboard.get(XWindowsClipboard::kText), m_testString);

  clipboard.add(XWindowsClipboard::kText, m_testString2);
  QCOMPARE(clipboard.get(XWindowsClipboard::kText), m_testString2);
}

XWindowsClipboard &XWindowsClipboardTests::getClipboard()
{
  return *m_clipboard;
}
#endif
QTEST_MAIN(XWindowsClipboardTests)

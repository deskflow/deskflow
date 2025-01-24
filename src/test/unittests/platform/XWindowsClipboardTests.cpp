/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboard.h"

#include "test/shared/undef_x11_macros.h"

#include <gtest/gtest.h>

const auto None = 0L;

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

TEST(XWindowsClipboardTests_CICCCMGetClipboard, ctor_default_errorNone)
{
  TestXWindowsClipboard::TestCICCCMGetClipboard clipboard;

  EXPECT_EQ(None, clipboard.error());
}

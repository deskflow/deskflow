/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

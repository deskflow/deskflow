/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
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

// TODO: fix tests - compile error on linux
#if 0

#include "platform/XWindowsClipboard.h"

#include "test/global/gtest.h"
#include <iostream>

class CXWindowsClipboardTests : public ::testing::Test
{
protected:
    virtual void
    SetUp()
    {
        m_display = XOpenDisplay(NULL);
        int screen = DefaultScreen(m_display);
        Window root = XRootWindow(m_display, screen);
        
        XSetWindowAttributes attr;
        attr.do_not_propagate_mask = 0;
        attr.override_redirect = True;
        attr.cursor = Cursor();
        
        m_window = XCreateWindow(
            m_display, root, 0, 0, 1, 1, 0, 0,
            InputOnly, CopyFromParent, 0, &attr);
    }

    virtual void
    TearDown()
    {
        XDestroyWindow(m_display, m_window);
        XCloseDisplay(m_display);
    }

    CXWindowsClipboard&
    createClipboard()
    {
        CXWindowsClipboard* clipboard;
        clipboard = new CXWindowsClipboard(m_display, m_window, 0);
        clipboard->open(0); // needed to empty the clipboard
        clipboard->empty(); // needed to own the clipboard
        return *clipboard;
    }

    Display* m_display;
    Window m_window;
};

TEST_F(CXWindowsClipboardTests, empty_openCalled_returnsTrue)
{
    CXWindowsClipboard clipboard = createClipboard();
    
    bool actual = clipboard.empty();
    
    EXPECT_EQ(true, actual);
}

TEST_F(CXWindowsClipboardTests, empty_singleFormat_hasReturnsFalse)
{
    CXWindowsClipboard clipboard = createClipboard();
    clipboard.add(CXWindowsClipboard::kText, "synergy rocks!");
    
    clipboard.empty();
    
    bool actual = clipboard.has(CXWindowsClipboard::kText);
    EXPECT_FALSE(actual);
}

TEST_F(CXWindowsClipboardTests, add_newValue_valueWasStored)
{
    CXWindowsClipboard clipboard = createClipboard();
    
    clipboard.add(IClipboard::kText, "synergy rocks!");
    
    String actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("synergy rocks!", actual);
}

TEST_F(CXWindowsClipboardTests, add_replaceValue_valueWasReplaced)
{
    CXWindowsClipboard clipboard = createClipboard();
    
    clipboard.add(IClipboard::kText, "synergy rocks!");
    clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.
    
    String actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("maxivista sucks", actual);
}

TEST_F(CXWindowsClipboardTests, close_isOpen_noErrors)
{
    CXWindowsClipboard clipboard = createClipboard();
    
    // clipboard opened in createClipboard()
    clipboard.close();
    
    // can't assert anything
}

TEST_F(CXWindowsClipboardTests, has_withFormatAdded_returnsTrue)
{
    CXWindowsClipboard clipboard = createClipboard();
    clipboard.add(IClipboard::kText, "synergy rocks!");
    
    bool actual = clipboard.has(IClipboard::kText);
    
    EXPECT_EQ(true, actual);
}

TEST_F(CXWindowsClipboardTests, has_withNoFormats_returnsFalse)
{
    CXWindowsClipboard clipboard = createClipboard();
    
    bool actual = clipboard.has(IClipboard::kText);
    
    EXPECT_FALSE(actual);
}

TEST_F(CXWindowsClipboardTests, get_withNoFormats_returnsEmpty)
{
    CXWindowsClipboard clipboard = createClipboard();
    
    String actual = clipboard.get(IClipboard::kText);
    
    EXPECT_EQ("", actual);
}

TEST_F(CXWindowsClipboardTests, get_withFormatAdded_returnsExpected)
{
    CXWindowsClipboard clipboard = createClipboard();
    clipboard.add(IClipboard::kText, "synergy rocks!");
    
    String actual = clipboard.get(IClipboard::kText);
    
    EXPECT_EQ("synergy rocks!", actual);
}

#endif

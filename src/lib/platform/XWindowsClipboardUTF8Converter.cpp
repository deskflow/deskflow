/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "platform/XWindowsClipboardUTF8Converter.h"

#include <algorithm>

//
// XWindowsClipboardUTF8Converter
//

XWindowsClipboardUTF8Converter::XWindowsClipboardUTF8Converter(
                Display* display, const char* name, bool normalize) :
    m_atom(XInternAtom(display, name, False)), m_normalize(normalize)
{
    // do nothing
}

XWindowsClipboardUTF8Converter::~XWindowsClipboardUTF8Converter()
{
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardUTF8Converter::getFormat() const
{
    return IClipboard::kText;
}

Atom
XWindowsClipboardUTF8Converter::getAtom() const
{
    return m_atom;
}

int
XWindowsClipboardUTF8Converter::getDataSize() const
{
    return 8;
}

static
bool
isCR(char ch)
{
    return (ch == '\r');
}

String
XWindowsClipboardUTF8Converter::fromIClipboard(const String& data) const
{
    return data;
}

String
XWindowsClipboardUTF8Converter::toIClipboard(const String& data) const
{
    // https://bugzilla.mozilla.org/show_bug.cgi?id=1547595
    // GTK normalizes the clipboard's line endings to CRLF (\r\n) internally.
    // When sending the raw data to other systems, like Windows, where \n is
    // converted to \r\n we end up with \r\r\n, resulting in double lines when
    // pasting.
    //
    // This seems to happen only when the clipboard format is
    // text/plain;charset=utf8 and not when it's UTF8_STRING.
    // When normalize clipboard is set, any \r present in the string is removed

    if (m_normalize) {
        String copy = data;

        copy.erase(std::remove_if(copy.begin(), copy.end(), isCR), copy.end());
        return copy;
    }

    return data;
}

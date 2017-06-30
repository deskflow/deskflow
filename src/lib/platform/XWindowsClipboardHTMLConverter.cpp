/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "platform/XWindowsClipboardHTMLConverter.h"

#include "base/Unicode.h"

//
// XWindowsClipboardHTMLConverter
//

XWindowsClipboardHTMLConverter::XWindowsClipboardHTMLConverter (
    Display* display, const char* name)
    : m_atom (XInternAtom (display, name, False)) {
    // do nothing
}

XWindowsClipboardHTMLConverter::~XWindowsClipboardHTMLConverter () {
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardHTMLConverter::getFormat () const {
    return IClipboard::kHTML;
}

Atom
XWindowsClipboardHTMLConverter::getAtom () const {
    return m_atom;
}

int
XWindowsClipboardHTMLConverter::getDataSize () const {
    return 8;
}

String
XWindowsClipboardHTMLConverter::fromIClipboard (const String& data) const {
    return Unicode::UTF8ToUTF16 (data);
}

String
XWindowsClipboardHTMLConverter::toIClipboard (const String& data) const {
    return Unicode::UTF16ToUTF8 (data);
}

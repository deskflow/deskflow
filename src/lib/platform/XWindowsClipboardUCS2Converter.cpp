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

#include "platform/XWindowsClipboardUCS2Converter.h"

#include "base/Unicode.h"

//
// XWindowsClipboardUCS2Converter
//

XWindowsClipboardUCS2Converter::XWindowsClipboardUCS2Converter (
    Display* display, const char* name)
    : m_atom (XInternAtom (display, name, False)) {
    // do nothing
}

XWindowsClipboardUCS2Converter::~XWindowsClipboardUCS2Converter () {
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardUCS2Converter::getFormat () const {
    return IClipboard::kText;
}

Atom
XWindowsClipboardUCS2Converter::getAtom () const {
    return m_atom;
}

int
XWindowsClipboardUCS2Converter::getDataSize () const {
    return 16;
}

String
XWindowsClipboardUCS2Converter::fromIClipboard (const String& data) const {
    return Unicode::UTF8ToUCS2 (data);
}

String
XWindowsClipboardUCS2Converter::toIClipboard (const String& data) const {
    return Unicode::UCS2ToUTF8 (data);
}

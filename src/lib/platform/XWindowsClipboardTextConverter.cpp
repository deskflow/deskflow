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

#include "platform/XWindowsClipboardTextConverter.h"

#include "base/Unicode.h"

//
// XWindowsClipboardTextConverter
//

XWindowsClipboardTextConverter::XWindowsClipboardTextConverter (
    Display* display, const char* name)
    : m_atom (XInternAtom (display, name, False)) {
    // do nothing
}

XWindowsClipboardTextConverter::~XWindowsClipboardTextConverter () {
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardTextConverter::getFormat () const {
    return IClipboard::kText;
}

Atom
XWindowsClipboardTextConverter::getAtom () const {
    return m_atom;
}

int
XWindowsClipboardTextConverter::getDataSize () const {
    return 8;
}

String
XWindowsClipboardTextConverter::fromIClipboard (const String& data) const {
    return Unicode::UTF8ToText (data);
}

String
XWindowsClipboardTextConverter::toIClipboard (const String& data) const {
    // convert to UTF-8
    bool errors;
    String utf8 = Unicode::textToUTF8 (data, &errors);

    // if there were decoding errors then, to support old applications
    // that don't understand UTF-8 but can report the exact binary
    // UTF-8 representation, see if the data appears to be UTF-8.  if
    // so then use it as is.
    if (errors && Unicode::isUTF8 (data)) {
        return data;
    }

    return utf8;
}

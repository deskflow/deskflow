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

#include "platform/MSWindowsClipboardTextConverter.h"

#include "base/Unicode.h"

//
// MSWindowsClipboardTextConverter
//

MSWindowsClipboardTextConverter::MSWindowsClipboardTextConverter () {
    // do nothing
}

MSWindowsClipboardTextConverter::~MSWindowsClipboardTextConverter () {
    // do nothing
}

UINT
MSWindowsClipboardTextConverter::getWin32Format () const {
    return CF_TEXT;
}

String
MSWindowsClipboardTextConverter::doFromIClipboard (const String& data) const {
    // convert and add nul terminator
    return Unicode::UTF8ToText (data) += '\0';
}

String
MSWindowsClipboardTextConverter::doToIClipboard (const String& data) const {
    // convert and truncate at first nul terminator
    String dst          = Unicode::textToUTF8 (data);
    String::size_type n = dst.find ('\0');
    if (n != String::npos) {
        dst.erase (n);
    }
    return dst;
}

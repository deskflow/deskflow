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

#include "platform/OSXClipboardUTF16Converter.h"

#include "base/Unicode.h"

//
// OSXClipboardUTF16Converter
//

OSXClipboardUTF16Converter::OSXClipboardUTF16Converter () {
    // do nothing
}

OSXClipboardUTF16Converter::~OSXClipboardUTF16Converter () {
    // do nothing
}

CFStringRef
OSXClipboardUTF16Converter::getOSXFormat () const {
    return CFSTR ("public.utf16-plain-text");
}

String
OSXClipboardUTF16Converter::doFromIClipboard (const String& data) const {
    // convert and add nul terminator
    return Unicode::UTF8ToUTF16 (data);
}

String
OSXClipboardUTF16Converter::doToIClipboard (const String& data) const {
    // convert and strip nul terminator
    return Unicode::UTF16ToUTF8 (data);
}

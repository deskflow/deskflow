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

#include "platform/OSXClipboardAnyTextConverter.h"

#include <algorithm>

//
// OSXClipboardAnyTextConverter
//

OSXClipboardAnyTextConverter::OSXClipboardAnyTextConverter () {
    // do nothing
}

OSXClipboardAnyTextConverter::~OSXClipboardAnyTextConverter () {
    // do nothing
}

IClipboard::EFormat
OSXClipboardAnyTextConverter::getFormat () const {
    return IClipboard::kText;
}

String
OSXClipboardAnyTextConverter::fromIClipboard (const String& data) const {
    // convert linefeeds and then convert to desired encoding
    return doFromIClipboard (convertLinefeedToMacOS (data));
}

String
OSXClipboardAnyTextConverter::toIClipboard (const String& data) const {
    // convert text then newlines
    return convertLinefeedToUnix (doToIClipboard (data));
}

static bool
isLF (char ch) {
    return (ch == '\n');
}

static bool
isCR (char ch) {
    return (ch == '\r');
}

String
OSXClipboardAnyTextConverter::convertLinefeedToMacOS (const String& src) {
    // note -- we assume src is a valid UTF-8 string
    String copy = src;

    std::replace_if (copy.begin (), copy.end (), isLF, '\r');

    return copy;
}

String
OSXClipboardAnyTextConverter::convertLinefeedToUnix (const String& src) {
    String copy = src;

    std::replace_if (copy.begin (), copy.end (), isCR, '\n');

    return copy;
}

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 * Patch by Ryan Chapman
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

#include "platform/OSXClipboardAnyBitmapConverter.h"
#include <algorithm>

OSXClipboardAnyBitmapConverter::OSXClipboardAnyBitmapConverter () {
    // do nothing
}

OSXClipboardAnyBitmapConverter::~OSXClipboardAnyBitmapConverter () {
    // do nothing
}

IClipboard::EFormat
OSXClipboardAnyBitmapConverter::getFormat () const {
    return IClipboard::kBitmap;
}

String
OSXClipboardAnyBitmapConverter::fromIClipboard (const String& data) const {
    return doFromIClipboard (data);
}

String
OSXClipboardAnyBitmapConverter::toIClipboard (const String& data) const {
    return doToIClipboard (data);
}

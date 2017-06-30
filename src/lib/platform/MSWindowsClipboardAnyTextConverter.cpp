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

#include "platform/MSWindowsClipboardAnyTextConverter.h"

//
// MSWindowsClipboardAnyTextConverter
//

MSWindowsClipboardAnyTextConverter::MSWindowsClipboardAnyTextConverter () {
    // do nothing
}

MSWindowsClipboardAnyTextConverter::~MSWindowsClipboardAnyTextConverter () {
    // do nothing
}

IClipboard::EFormat
MSWindowsClipboardAnyTextConverter::getFormat () const {
    return IClipboard::kText;
}

HANDLE
MSWindowsClipboardAnyTextConverter::fromIClipboard (const String& data) const {
    // convert linefeeds and then convert to desired encoding
    String text = doFromIClipboard (convertLinefeedToWin32 (data));
    UInt32 size = (UInt32) text.size ();

    // copy to memory handle
    HGLOBAL gData = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE, size);
    if (gData != NULL) {
        // get a pointer to the allocated memory
        char* dst = (char*) GlobalLock (gData);
        if (dst != NULL) {
            memcpy (dst, text.data (), size);
            GlobalUnlock (gData);
        } else {
            GlobalFree (gData);
            gData = NULL;
        }
    }

    return gData;
}

String
MSWindowsClipboardAnyTextConverter::toIClipboard (HANDLE data) const {
    // get datator
    const char* src = (const char*) GlobalLock (data);
    UInt32 srcSize  = (UInt32) GlobalSize (data);
    if (src == NULL || srcSize <= 1) {
        return String ();
    }

    // convert text
    String text = doToIClipboard (String (src, srcSize));

    // release handle
    GlobalUnlock (data);

    // convert newlines
    return convertLinefeedToUnix (text);
}

String
MSWindowsClipboardAnyTextConverter::convertLinefeedToWin32 (
    const String& src) const {
    // note -- we assume src is a valid UTF-8 string

    // count newlines in string
    UInt32 numNewlines = 0;
    UInt32 n           = (UInt32) src.size ();
    for (const char *scan = src.c_str (); n > 0; ++scan, --n) {
        if (*scan == '\n') {
            ++numNewlines;
        }
    }
    if (numNewlines == 0) {
        return src;
    }

    // allocate new string
    String dst;
    dst.reserve (src.size () + numNewlines);

    // copy string, converting newlines
    n = (UInt32) src.size ();
    for (const char *scan = src.c_str (); n > 0; ++scan, --n) {
        if (scan[0] == '\n') {
            dst += '\r';
        }
        dst += scan[0];
    }

    return dst;
}

String
MSWindowsClipboardAnyTextConverter::convertLinefeedToUnix (
    const String& src) const {
    // count newlines in string
    UInt32 numNewlines = 0;
    UInt32 n           = (UInt32) src.size ();
    for (const char *scan = src.c_str (); n > 0; ++scan, --n) {
        if (scan[0] == '\r' && scan[1] == '\n') {
            ++numNewlines;
        }
    }
    if (numNewlines == 0) {
        return src;
    }

    // allocate new string
    String dst;
    dst.reserve (src.size ());

    // copy string, converting newlines
    n = (UInt32) src.size ();
    for (const char *scan = src.c_str (); n > 0; ++scan, --n) {
        if (scan[0] != '\r' || scan[1] != '\n') {
            dst += scan[0];
        }
    }

    return dst;
}

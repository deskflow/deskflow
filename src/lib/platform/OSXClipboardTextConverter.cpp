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

#include "platform/OSXClipboardTextConverter.h"

#include "base/Unicode.h"

//
// OSXClipboardTextConverter
//

OSXClipboardTextConverter::OSXClipboardTextConverter () {
    // do nothing
}

OSXClipboardTextConverter::~OSXClipboardTextConverter () {
    // do nothing
}

CFStringRef
OSXClipboardTextConverter::getOSXFormat () const {
    return CFSTR ("public.plain-text");
}

String
OSXClipboardTextConverter::convertString (const String& data,
                                          CFStringEncoding fromEncoding,
                                          CFStringEncoding toEncoding) {
    CFStringRef stringRef = CFStringCreateWithCString (
        kCFAllocatorDefault, data.c_str (), fromEncoding);

    if (stringRef == NULL) {
        return String ();
    }

    CFIndex buffSize;
    CFRange entireString = CFRangeMake (0, CFStringGetLength (stringRef));

    CFStringGetBytes (
        stringRef, entireString, toEncoding, 0, false, NULL, 0, &buffSize);

    char* buffer = new char[buffSize];

    if (buffer == NULL) {
        CFRelease (stringRef);
        return String ();
    }

    CFStringGetBytes (stringRef,
                      entireString,
                      toEncoding,
                      0,
                      false,
                      (UInt8*) buffer,
                      buffSize,
                      NULL);

    String result (buffer, buffSize);

    delete[] buffer;
    CFRelease (stringRef);

    return result;
}

String
OSXClipboardTextConverter::doFromIClipboard (const String& data) const {
    return convertString (
        data, kCFStringEncodingUTF8, CFStringGetSystemEncoding ());
}

String
OSXClipboardTextConverter::doToIClipboard (const String& data) const {
    return convertString (
        data, CFStringGetSystemEncoding (), kCFStringEncodingUTF8);
}

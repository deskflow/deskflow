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

#include "platform/OSXClipboardHTMLConverter.h"

#include "base/Unicode.h"

OSXClipboardHTMLConverter::OSXClipboardHTMLConverter () {
    // do nothing
}

OSXClipboardHTMLConverter::~OSXClipboardHTMLConverter () {
    // do nothing
}

IClipboard::EFormat
OSXClipboardHTMLConverter::getFormat () const {
    return IClipboard::kHTML;
}

CFStringRef
OSXClipboardHTMLConverter::getOSXFormat () const {
    return CFSTR ("public.html");
}

String
OSXClipboardHTMLConverter::convertString (const String& data,
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
OSXClipboardHTMLConverter::doFromIClipboard (const String& data) const {
    return convertString (
        data, kCFStringEncodingUTF8, CFStringGetSystemEncoding ());
}

String
OSXClipboardHTMLConverter::doToIClipboard (const String& data) const {
    return convertString (
        data, CFStringGetSystemEncoding (), kCFStringEncodingUTF8);
}

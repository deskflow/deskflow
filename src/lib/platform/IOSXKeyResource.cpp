/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#include "platform/IOSXKeyResource.h"

#include <Carbon/Carbon.h>

KeyID
IOSXKeyResource::getKeyID (UInt8 c) {
    if (c == 0) {
        return kKeyNone;
    } else if (c >= 32 && c < 127) {
        // ASCII
        return static_cast<KeyID> (c);
    } else {
        // handle special keys
        switch (c) {
            case 0x01:
                return kKeyHome;

            case 0x02:
                return kKeyKP_Enter;

            case 0x03:
                return kKeyKP_Enter;

            case 0x04:
                return kKeyEnd;

            case 0x05:
                return kKeyHelp;

            case 0x08:
                return kKeyBackSpace;

            case 0x09:
                return kKeyTab;

            case 0x0b:
                return kKeyPageUp;

            case 0x0c:
                return kKeyPageDown;

            case 0x0d:
                return kKeyReturn;

            case 0x10:
                // OS X maps all the function keys (F1, etc) to this one key.
                // we can't determine the right key here so we have to do it
                // some other way.
                return kKeyNone;

            case 0x1b:
                return kKeyEscape;

            case 0x1c:
                return kKeyLeft;

            case 0x1d:
                return kKeyRight;

            case 0x1e:
                return kKeyUp;

            case 0x1f:
                return kKeyDown;

            case 0x7f:
                return kKeyDelete;

            case 0x06:
            case 0x07:
            case 0x0a:
            case 0x0e:
            case 0x0f:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x19:
            case 0x1a:
                // discard other control characters
                return kKeyNone;

            default:
                // not special or unknown
                break;
        }

        // create string with character
        char str[2];
        str[0] = static_cast<char> (c);
        str[1] = 0;

        // get current keyboard script
        TISInputSourceRef isref = TISCopyCurrentKeyboardInputSource ();
        CFArrayRef langs        = (CFArrayRef) TISGetInputSourceProperty (
            isref, kTISPropertyInputSourceLanguages);
        CFStringEncoding encoding = CFStringConvertIANACharSetNameToEncoding (
            (CFStringRef) CFArrayGetValueAtIndex (langs, 0));
        // convert to unicode
        CFStringRef cfString = CFStringCreateWithCStringNoCopy (
            kCFAllocatorDefault, str, encoding, kCFAllocatorNull);

        // sometimes CFStringCreate...() returns NULL (e.g. Apple Korean
        // encoding with char value 214).  if it did then make no key,
        // otherwise CFStringCreateMutableCopy() will crash.
        if (cfString == NULL) {
            return kKeyNone;
        }

        // convert to precomposed
        CFMutableStringRef mcfString =
            CFStringCreateMutableCopy (kCFAllocatorDefault, 0, cfString);
        CFRelease (cfString);
        CFStringNormalize (mcfString, kCFStringNormalizationFormC);

        // check result
        int unicodeLength = CFStringGetLength (mcfString);
        if (unicodeLength == 0) {
            CFRelease (mcfString);
            return kKeyNone;
        }
        if (unicodeLength > 1) {
            // FIXME -- more than one character, we should handle this
            CFRelease (mcfString);
            return kKeyNone;
        }

        // get unicode character
        UniChar uc = CFStringGetCharacterAtIndex (mcfString, 0);
        CFRelease (mcfString);

        // convert to KeyID
        return static_cast<KeyID> (uc);
    }
}

KeyID
IOSXKeyResource::unicharToKeyID (UniChar c) {
    switch (c) {
        case 3:
            return kKeyKP_Enter;

        case 8:
            return kKeyBackSpace;

        case 9:
            return kKeyTab;

        case 13:
            return kKeyReturn;

        case 27:
            return kKeyEscape;

        case 127:
            return kKeyDelete;

        default:
            if (c < 32) {
                return kKeyNone;
            }
            return static_cast<KeyID> (c);
    }
}

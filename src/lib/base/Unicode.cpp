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

#include "arch/Arch.h"
#include "base/Unicode.h"

#include <cstring>

//
// local utility functions
//

inline static UInt16
decode16 (const UInt8* n, bool byteSwapped) {
    union x16 {
        UInt8 n8[2];
        UInt16 n16;
    } c;
    if (byteSwapped) {
        c.n8[0] = n[1];
        c.n8[1] = n[0];
    } else {
        c.n8[0] = n[0];
        c.n8[1] = n[1];
    }
    return c.n16;
}

inline static UInt32
decode32 (const UInt8* n, bool byteSwapped) {
    union x32 {
        UInt8 n8[4];
        UInt32 n32;
    } c;
    if (byteSwapped) {
        c.n8[0] = n[3];
        c.n8[1] = n[2];
        c.n8[2] = n[1];
        c.n8[3] = n[0];
    } else {
        c.n8[0] = n[0];
        c.n8[1] = n[1];
        c.n8[2] = n[2];
        c.n8[3] = n[3];
    }
    return c.n32;
}

inline static void
resetError (bool* errors) {
    if (errors != NULL) {
        *errors = false;
    }
}

inline static void
setError (bool* errors) {
    if (errors != NULL) {
        *errors = true;
    }
}


//
// Unicode
//

UInt32 Unicode::s_invalid     = 0x0000ffff;
UInt32 Unicode::s_replacement = 0x0000fffd;

bool
Unicode::isUTF8 (const String& src) {
    // convert and test each character
    const UInt8* data = reinterpret_cast<const UInt8*> (src.c_str ());
    for (UInt32 n = (UInt32) src.size (); n > 0;) {
        if (fromUTF8 (data, n) == s_invalid) {
            return false;
        }
    }
    return true;
}

String
Unicode::UTF8ToUCS2 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // get size of input string and reserve some space in output
    UInt32 n = (UInt32) src.size ();
    String dst;
    dst.reserve (2 * n);

    // convert each character
    const UInt8* data = reinterpret_cast<const UInt8*> (src.c_str ());
    while (n > 0) {
        UInt32 c = fromUTF8 (data, n);
        if (c == s_invalid) {
            c = s_replacement;
        } else if (c >= 0x00010000) {
            setError (errors);
            c = s_replacement;
        }
        UInt16 ucs2 = static_cast<UInt16> (c);
        dst.append (reinterpret_cast<const char*> (&ucs2), 2);
    }

    return dst;
}

String
Unicode::UTF8ToUCS4 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // get size of input string and reserve some space in output
    UInt32 n = (UInt32) src.size ();
    String dst;
    dst.reserve (4 * n);

    // convert each character
    const UInt8* data = reinterpret_cast<const UInt8*> (src.c_str ());
    while (n > 0) {
        UInt32 c = fromUTF8 (data, n);
        if (c == s_invalid) {
            c = s_replacement;
        }
        dst.append (reinterpret_cast<const char*> (&c), 4);
    }

    return dst;
}

String
Unicode::UTF8ToUTF16 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // get size of input string and reserve some space in output
    UInt32 n = (UInt32) src.size ();
    String dst;
    dst.reserve (2 * n);

    // convert each character
    const UInt8* data = reinterpret_cast<const UInt8*> (src.c_str ());
    while (n > 0) {
        UInt32 c = fromUTF8 (data, n);
        if (c == s_invalid) {
            c = s_replacement;
        } else if (c >= 0x00110000) {
            setError (errors);
            c = s_replacement;
        }
        if (c < 0x00010000) {
            UInt16 ucs2 = static_cast<UInt16> (c);
            dst.append (reinterpret_cast<const char*> (&ucs2), 2);
        } else {
            c -= 0x00010000;
            UInt16 utf16h = static_cast<UInt16> ((c >> 10) + 0xd800);
            UInt16 utf16l = static_cast<UInt16> ((c & 0x03ff) + 0xdc00);
            dst.append (reinterpret_cast<const char*> (&utf16h), 2);
            dst.append (reinterpret_cast<const char*> (&utf16l), 2);
        }
    }

    return dst;
}

String
Unicode::UTF8ToUTF32 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // get size of input string and reserve some space in output
    UInt32 n = (UInt32) src.size ();
    String dst;
    dst.reserve (4 * n);

    // convert each character
    const UInt8* data = reinterpret_cast<const UInt8*> (src.c_str ());
    while (n > 0) {
        UInt32 c = fromUTF8 (data, n);
        if (c == s_invalid) {
            c = s_replacement;
        } else if (c >= 0x00110000) {
            setError (errors);
            c = s_replacement;
        }
        dst.append (reinterpret_cast<const char*> (&c), 4);
    }

    return dst;
}

String
Unicode::UTF8ToText (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // convert to wide char
    UInt32 size;
    wchar_t* tmp = UTF8ToWideChar (src, size, errors);

    // convert string to multibyte
    int len   = ARCH->convStringWCToMB (NULL, tmp, size, errors);
    char* mbs = new char[len + 1];
    ARCH->convStringWCToMB (mbs, tmp, size, errors);
    String text (mbs, len);

    // clean up
    delete[] mbs;
    delete[] tmp;

    return text;
}

String
Unicode::UCS2ToUTF8 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // convert
    UInt32 n = (UInt32) src.size () >> 1;
    return doUCS2ToUTF8 (
        reinterpret_cast<const UInt8*> (src.data ()), n, errors);
}

String
Unicode::UCS4ToUTF8 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // convert
    UInt32 n = (UInt32) src.size () >> 2;
    return doUCS4ToUTF8 (
        reinterpret_cast<const UInt8*> (src.data ()), n, errors);
}

String
Unicode::UTF16ToUTF8 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // convert
    UInt32 n = (UInt32) src.size () >> 1;
    return doUTF16ToUTF8 (
        reinterpret_cast<const UInt8*> (src.data ()), n, errors);
}

String
Unicode::UTF32ToUTF8 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // convert
    UInt32 n = (UInt32) src.size () >> 2;
    return doUTF32ToUTF8 (
        reinterpret_cast<const UInt8*> (src.data ()), n, errors);
}

String
Unicode::textToUTF8 (const String& src, bool* errors) {
    // default to success
    resetError (errors);

    // convert string to wide characters
    UInt32 n     = (UInt32) src.size ();
    int len      = ARCH->convStringMBToWC (NULL, src.c_str (), n, errors);
    wchar_t* wcs = new wchar_t[len + 1];
    ARCH->convStringMBToWC (wcs, src.c_str (), n, errors);

    // convert to UTF8
    String utf8 = wideCharToUTF8 (wcs, len, errors);

    // clean up
    delete[] wcs;

    return utf8;
}

wchar_t*
Unicode::UTF8ToWideChar (const String& src, UInt32& size, bool* errors) {
    // convert to platform's wide character encoding
    String tmp;
    switch (ARCH->getWideCharEncoding ()) {
        case IArchString::kUCS2:
            tmp  = UTF8ToUCS2 (src, errors);
            size = (UInt32) tmp.size () >> 1;
            break;

        case IArchString::kUCS4:
            tmp  = UTF8ToUCS4 (src, errors);
            size = (UInt32) tmp.size () >> 2;
            break;

        case IArchString::kUTF16:
            tmp  = UTF8ToUTF16 (src, errors);
            size = (UInt32) tmp.size () >> 1;
            break;

        case IArchString::kUTF32:
            tmp  = UTF8ToUTF32 (src, errors);
            size = (UInt32) tmp.size () >> 2;
            break;

        default:
            assert (0 && "unknown wide character encoding");
    }

    // copy to a wchar_t array
    wchar_t* dst = new wchar_t[size];
    ::memcpy (dst, tmp.data (), sizeof (wchar_t) * size);
    return dst;
}

String
Unicode::wideCharToUTF8 (const wchar_t* src, UInt32 size, bool* errors) {
    // convert from platform's wide character encoding.
    // note -- this must include a wide nul character (independent of
    // the String's nul character).
    switch (ARCH->getWideCharEncoding ()) {
        case IArchString::kUCS2:
            return doUCS2ToUTF8 (
                reinterpret_cast<const UInt8*> (src), size, errors);

        case IArchString::kUCS4:
            return doUCS4ToUTF8 (
                reinterpret_cast<const UInt8*> (src), size, errors);

        case IArchString::kUTF16:
            return doUTF16ToUTF8 (
                reinterpret_cast<const UInt8*> (src), size, errors);

        case IArchString::kUTF32:
            return doUTF32ToUTF8 (
                reinterpret_cast<const UInt8*> (src), size, errors);

        default:
            assert (0 && "unknown wide character encoding");
            return String ();
    }
}

String
Unicode::doUCS2ToUTF8 (const UInt8* data, UInt32 n, bool* errors) {
    // make some space
    String dst;
    dst.reserve (n);

    // check if first character is 0xfffe or 0xfeff
    bool byteSwapped = false;
    if (n >= 1) {
        switch (decode16 (data, false)) {
            case 0x0000feff:
                data += 2;
                --n;
                break;

            case 0x0000fffe:
                byteSwapped = true;
                data += 2;
                --n;
                break;

            default:
                break;
        }
    }

    // convert each character
    for (; n > 0; data += 2, --n) {
        UInt32 c = decode16 (data, byteSwapped);
        toUTF8 (dst, c, errors);
    }

    return dst;
}

String
Unicode::doUCS4ToUTF8 (const UInt8* data, UInt32 n, bool* errors) {
    // make some space
    String dst;
    dst.reserve (n);

    // check if first character is 0xfffe or 0xfeff
    bool byteSwapped = false;
    if (n >= 1) {
        switch (decode32 (data, false)) {
            case 0x0000feff:
                data += 4;
                --n;
                break;

            case 0x0000fffe:
                byteSwapped = true;
                data += 4;
                --n;
                break;

            default:
                break;
        }
    }

    // convert each character
    for (; n > 0; data += 4, --n) {
        UInt32 c = decode32 (data, byteSwapped);
        toUTF8 (dst, c, errors);
    }

    return dst;
}

String
Unicode::doUTF16ToUTF8 (const UInt8* data, UInt32 n, bool* errors) {
    // make some space
    String dst;
    dst.reserve (n);

    // check if first character is 0xfffe or 0xfeff
    bool byteSwapped = false;
    if (n >= 1) {
        switch (decode16 (data, false)) {
            case 0x0000feff:
                data += 2;
                --n;
                break;

            case 0x0000fffe:
                byteSwapped = true;
                data += 2;
                --n;
                break;

            default:
                break;
        }
    }

    // convert each character
    for (; n > 0; data += 2, --n) {
        UInt32 c = decode16 (data, byteSwapped);
        if (c < 0x0000d800 || c > 0x0000dfff) {
            toUTF8 (dst, c, errors);
        } else if (n == 1) {
            // error -- missing second word
            setError (errors);
            toUTF8 (dst, s_replacement, NULL);
        } else if (c >= 0x0000d800 && c <= 0x0000dbff) {
            UInt32 c2 = decode16 (data, byteSwapped);
            data += 2;
            --n;
            if (c2 < 0x0000dc00 || c2 > 0x0000dfff) {
                // error -- [d800,dbff] not followed by [dc00,dfff]
                setError (errors);
                toUTF8 (dst, s_replacement, NULL);
            } else {
                c = (((c - 0x0000d800) << 10) | (c2 - 0x0000dc00)) + 0x00010000;
                toUTF8 (dst, c, errors);
            }
        } else {
            // error -- [dc00,dfff] without leading [d800,dbff]
            setError (errors);
            toUTF8 (dst, s_replacement, NULL);
        }
    }

    return dst;
}

String
Unicode::doUTF32ToUTF8 (const UInt8* data, UInt32 n, bool* errors) {
    // make some space
    String dst;
    dst.reserve (n);

    // check if first character is 0xfffe or 0xfeff
    bool byteSwapped = false;
    if (n >= 1) {
        switch (decode32 (data, false)) {
            case 0x0000feff:
                data += 4;
                --n;
                break;

            case 0x0000fffe:
                byteSwapped = true;
                data += 4;
                --n;
                break;

            default:
                break;
        }
    }

    // convert each character
    for (; n > 0; data += 4, --n) {
        UInt32 c = decode32 (data, byteSwapped);
        if (c >= 0x00110000) {
            setError (errors);
            c = s_replacement;
        }
        toUTF8 (dst, c, errors);
    }

    return dst;
}

UInt32
Unicode::fromUTF8 (const UInt8*& data, UInt32& n) {
    assert (data != NULL);
    assert (n != 0);

    // compute character encoding length, checking for overlong
    // sequences (i.e. characters that don't use the shortest
    // possible encoding).
    UInt32 size;
    if (data[0] < 0x80) {
        // 0xxxxxxx
        size = 1;
    } else if (data[0] < 0xc0) {
        // 10xxxxxx -- in the middle of a multibyte character.  counts
        // as one invalid character.
        --n;
        ++data;
        return s_invalid;
    } else if (data[0] < 0xe0) {
        // 110xxxxx
        size = 2;
    } else if (data[0] < 0xf0) {
        // 1110xxxx
        size = 3;
    } else if (data[0] < 0xf8) {
        // 11110xxx
        size = 4;
    } else if (data[0] < 0xfc) {
        // 111110xx
        size = 5;
    } else if (data[0] < 0xfe) {
        // 1111110x
        size = 6;
    } else {
        // invalid sequence.  dunno how many bytes to skip so skip one.
        --n;
        ++data;
        return s_invalid;
    }

    // make sure we have enough data
    if (size > n) {
        data += n;
        n = 0;
        return s_invalid;
    }

    // extract character
    UInt32 c;
    switch (size) {
        case 1:
            c = static_cast<UInt32> (data[0]);
            break;

        case 2:
            c = ((static_cast<UInt32> (data[0]) & 0x1f) << 6) |
                ((static_cast<UInt32> (data[1]) & 0x3f));
            break;

        case 3:
            c = ((static_cast<UInt32> (data[0]) & 0x0f) << 12) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 6) |
                ((static_cast<UInt32> (data[2]) & 0x3f));
            break;

        case 4:
            c = ((static_cast<UInt32> (data[0]) & 0x07) << 18) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 12) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 6) |
                ((static_cast<UInt32> (data[1]) & 0x3f));
            break;

        case 5:
            c = ((static_cast<UInt32> (data[0]) & 0x03) << 24) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 18) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 12) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 6) |
                ((static_cast<UInt32> (data[1]) & 0x3f));
            break;

        case 6:
            c = ((static_cast<UInt32> (data[0]) & 0x01) << 30) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 24) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 18) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 12) |
                ((static_cast<UInt32> (data[1]) & 0x3f) << 6) |
                ((static_cast<UInt32> (data[1]) & 0x3f));
            break;

        default:
            assert (0 && "invalid size");
            return s_invalid;
    }

    // check that all bytes after the first have the pattern 10xxxxxx.
    // truncated sequences are treated as a single malformed character.
    bool truncated = false;
    switch (size) {
        case 6:
            if ((data[5] & 0xc0) != 0x80) {
                truncated = true;
                size      = 5;
            }
        // fall through

        case 5:
            if ((data[4] & 0xc0) != 0x80) {
                truncated = true;
                size      = 4;
            }
        // fall through

        case 4:
            if ((data[3] & 0xc0) != 0x80) {
                truncated = true;
                size      = 3;
            }
        // fall through

        case 3:
            if ((data[2] & 0xc0) != 0x80) {
                truncated = true;
                size      = 2;
            }
        // fall through

        case 2:
            if ((data[1] & 0xc0) != 0x80) {
                truncated = true;
                size      = 1;
            }
    }

    // update parameters
    data += size;
    n -= size;

    // invalid if sequence was truncated
    if (truncated) {
        return s_invalid;
    }

    // check for characters that didn't use the smallest possible encoding
    static UInt32 s_minChar[] = {0,
                                 0x00000000,
                                 0x00000080,
                                 0x00000800,
                                 0x00010000,
                                 0x00200000,
                                 0x04000000};
    if (c < s_minChar[size]) {
        return s_invalid;
    }

    // check for characters not in ISO-10646
    if (c >= 0x0000d800 && c <= 0x0000dfff) {
        return s_invalid;
    }
    if (c >= 0x0000fffe && c <= 0x0000ffff) {
        return s_invalid;
    }

    return c;
}

void
Unicode::toUTF8 (String& dst, UInt32 c, bool* errors) {
    UInt8 data[6];

    // handle characters outside the valid range
    if ((c >= 0x0000d800 && c <= 0x0000dfff) || c >= 0x80000000) {
        setError (errors);
        c = s_replacement;
    }

    // convert to UTF-8
    if (c < 0x00000080) {
        data[0] = static_cast<UInt8> (c);
        dst.append (reinterpret_cast<char*> (data), 1);
    } else if (c < 0x00000800) {
        data[0] = static_cast<UInt8> (((c >> 6) & 0x0000001f) + 0xc0);
        data[1] = static_cast<UInt8> ((c & 0x0000003f) + 0x80);
        dst.append (reinterpret_cast<char*> (data), 2);
    } else if (c < 0x00010000) {
        data[0] = static_cast<UInt8> (((c >> 12) & 0x0000000f) + 0xe0);
        data[1] = static_cast<UInt8> (((c >> 6) & 0x0000003f) + 0x80);
        data[2] = static_cast<UInt8> ((c & 0x0000003f) + 0x80);
        dst.append (reinterpret_cast<char*> (data), 3);
    } else if (c < 0x00200000) {
        data[0] = static_cast<UInt8> (((c >> 18) & 0x00000007) + 0xf0);
        data[1] = static_cast<UInt8> (((c >> 12) & 0x0000003f) + 0x80);
        data[2] = static_cast<UInt8> (((c >> 6) & 0x0000003f) + 0x80);
        data[3] = static_cast<UInt8> ((c & 0x0000003f) + 0x80);
        dst.append (reinterpret_cast<char*> (data), 4);
    } else if (c < 0x04000000) {
        data[0] = static_cast<UInt8> (((c >> 24) & 0x00000003) + 0xf8);
        data[1] = static_cast<UInt8> (((c >> 18) & 0x0000003f) + 0x80);
        data[2] = static_cast<UInt8> (((c >> 12) & 0x0000003f) + 0x80);
        data[3] = static_cast<UInt8> (((c >> 6) & 0x0000003f) + 0x80);
        data[4] = static_cast<UInt8> ((c & 0x0000003f) + 0x80);
        dst.append (reinterpret_cast<char*> (data), 5);
    } else if (c < 0x80000000) {
        data[0] = static_cast<UInt8> (((c >> 30) & 0x00000001) + 0xfc);
        data[1] = static_cast<UInt8> (((c >> 24) & 0x0000003f) + 0x80);
        data[2] = static_cast<UInt8> (((c >> 18) & 0x0000003f) + 0x80);
        data[3] = static_cast<UInt8> (((c >> 12) & 0x0000003f) + 0x80);
        data[4] = static_cast<UInt8> (((c >> 6) & 0x0000003f) + 0x80);
        data[5] = static_cast<UInt8> ((c & 0x0000003f) + 0x80);
        dst.append (reinterpret_cast<char*> (data), 6);
    } else {
        assert (0 && "character out of range");
    }
}

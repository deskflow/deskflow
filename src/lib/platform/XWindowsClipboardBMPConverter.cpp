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

#include "platform/XWindowsClipboardBMPConverter.h"

// BMP file header structure
struct CBMPHeader {
public:
    UInt16 type;
    UInt32 size;
    UInt16 reserved1;
    UInt16 reserved2;
    UInt32 offset;
};

// BMP is little-endian
static inline UInt32
fromLEU32 (const UInt8* data) {
    return static_cast<UInt32> (data[0]) |
           (static_cast<UInt32> (data[1]) << 8) |
           (static_cast<UInt32> (data[2]) << 16) |
           (static_cast<UInt32> (data[3]) << 24);
}

static void
toLE (UInt8*& dst, char src) {
    dst[0] = static_cast<UInt8> (src);
    dst += 1;
}

static void
toLE (UInt8*& dst, UInt16 src) {
    dst[0] = static_cast<UInt8> (src & 0xffu);
    dst[1] = static_cast<UInt8> ((src >> 8) & 0xffu);
    dst += 2;
}

static void
toLE (UInt8*& dst, UInt32 src) {
    dst[0] = static_cast<UInt8> (src & 0xffu);
    dst[1] = static_cast<UInt8> ((src >> 8) & 0xffu);
    dst[2] = static_cast<UInt8> ((src >> 16) & 0xffu);
    dst[3] = static_cast<UInt8> ((src >> 24) & 0xffu);
    dst += 4;
}

//
// XWindowsClipboardBMPConverter
//

XWindowsClipboardBMPConverter::XWindowsClipboardBMPConverter (Display* display)
    : m_atom (XInternAtom (display, "image/bmp", False)) {
    // do nothing
}

XWindowsClipboardBMPConverter::~XWindowsClipboardBMPConverter () {
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardBMPConverter::getFormat () const {
    return IClipboard::kBitmap;
}

Atom
XWindowsClipboardBMPConverter::getAtom () const {
    return m_atom;
}

int
XWindowsClipboardBMPConverter::getDataSize () const {
    return 8;
}

String
XWindowsClipboardBMPConverter::fromIClipboard (const String& bmp) const {
    // create BMP image
    UInt8 header[14];
    UInt8* dst = header;
    toLE (dst, 'B');
    toLE (dst, 'M');
    toLE (dst, static_cast<UInt32> (14 + bmp.size ()));
    toLE (dst, static_cast<UInt16> (0));
    toLE (dst, static_cast<UInt16> (0));
    toLE (dst, static_cast<UInt32> (14 + 40));
    return String (reinterpret_cast<const char*> (header), 14) + bmp;
}

String
XWindowsClipboardBMPConverter::toIClipboard (const String& bmp) const {
    // make sure data is big enough for a BMP file
    if (bmp.size () <= 14 + 40) {
        return String ();
    }

    // check BMP file header
    const UInt8* rawBMPHeader = reinterpret_cast<const UInt8*> (bmp.data ());
    if (rawBMPHeader[0] != 'B' || rawBMPHeader[1] != 'M') {
        return String ();
    }

    // get offset to image data
    UInt32 offset = fromLEU32 (rawBMPHeader + 10);

    // construct BMP
    if (offset == 14 + 40) {
        return bmp.substr (14);
    } else {
        return bmp.substr (14, 40) + bmp.substr (offset, bmp.size () - offset);
    }
}

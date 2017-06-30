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

#include "platform/XWindowsClipboardAnyBitmapConverter.h"

// BMP info header structure
struct CBMPInfoHeader {
public:
    UInt32 biSize;
    SInt32 biWidth;
    SInt32 biHeight;
    UInt16 biPlanes;
    UInt16 biBitCount;
    UInt32 biCompression;
    UInt32 biSizeImage;
    SInt32 biXPelsPerMeter;
    SInt32 biYPelsPerMeter;
    UInt32 biClrUsed;
    UInt32 biClrImportant;
};

// BMP is little-endian

static void
toLE (UInt8*& dst, UInt16 src) {
    dst[0] = static_cast<UInt8> (src & 0xffu);
    dst[1] = static_cast<UInt8> ((src >> 8) & 0xffu);
    dst += 2;
}

static void
toLE (UInt8*& dst, SInt32 src) {
    dst[0] = static_cast<UInt8> (src & 0xffu);
    dst[1] = static_cast<UInt8> ((src >> 8) & 0xffu);
    dst[2] = static_cast<UInt8> ((src >> 16) & 0xffu);
    dst[3] = static_cast<UInt8> ((src >> 24) & 0xffu);
    dst += 4;
}

static void
toLE (UInt8*& dst, UInt32 src) {
    dst[0] = static_cast<UInt8> (src & 0xffu);
    dst[1] = static_cast<UInt8> ((src >> 8) & 0xffu);
    dst[2] = static_cast<UInt8> ((src >> 16) & 0xffu);
    dst[3] = static_cast<UInt8> ((src >> 24) & 0xffu);
    dst += 4;
}

static inline UInt16
fromLEU16 (const UInt8* data) {
    return static_cast<UInt16> (data[0]) | (static_cast<UInt16> (data[1]) << 8);
}

static inline SInt32
fromLES32 (const UInt8* data) {
    return static_cast<SInt32> (static_cast<UInt32> (data[0]) |
                                (static_cast<UInt32> (data[1]) << 8) |
                                (static_cast<UInt32> (data[2]) << 16) |
                                (static_cast<UInt32> (data[3]) << 24));
}

static inline UInt32
fromLEU32 (const UInt8* data) {
    return static_cast<UInt32> (data[0]) |
           (static_cast<UInt32> (data[1]) << 8) |
           (static_cast<UInt32> (data[2]) << 16) |
           (static_cast<UInt32> (data[3]) << 24);
}


//
// XWindowsClipboardAnyBitmapConverter
//

XWindowsClipboardAnyBitmapConverter::XWindowsClipboardAnyBitmapConverter () {
    // do nothing
}

XWindowsClipboardAnyBitmapConverter::~XWindowsClipboardAnyBitmapConverter () {
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardAnyBitmapConverter::getFormat () const {
    return IClipboard::kBitmap;
}

int
XWindowsClipboardAnyBitmapConverter::getDataSize () const {
    return 8;
}

String
XWindowsClipboardAnyBitmapConverter::fromIClipboard (const String& bmp) const {
    // fill BMP info header with native-endian data
    CBMPInfoHeader infoHeader;
    const UInt8* rawBMPInfoHeader =
        reinterpret_cast<const UInt8*> (bmp.data ());
    infoHeader.biSize          = fromLEU32 (rawBMPInfoHeader + 0);
    infoHeader.biWidth         = fromLES32 (rawBMPInfoHeader + 4);
    infoHeader.biHeight        = fromLES32 (rawBMPInfoHeader + 8);
    infoHeader.biPlanes        = fromLEU16 (rawBMPInfoHeader + 12);
    infoHeader.biBitCount      = fromLEU16 (rawBMPInfoHeader + 14);
    infoHeader.biCompression   = fromLEU32 (rawBMPInfoHeader + 16);
    infoHeader.biSizeImage     = fromLEU32 (rawBMPInfoHeader + 20);
    infoHeader.biXPelsPerMeter = fromLES32 (rawBMPInfoHeader + 24);
    infoHeader.biYPelsPerMeter = fromLES32 (rawBMPInfoHeader + 28);
    infoHeader.biClrUsed       = fromLEU32 (rawBMPInfoHeader + 32);
    infoHeader.biClrImportant  = fromLEU32 (rawBMPInfoHeader + 36);

    // check that format is acceptable
    if (infoHeader.biSize != 40 || infoHeader.biWidth == 0 ||
        infoHeader.biHeight == 0 || infoHeader.biPlanes != 0 ||
        infoHeader.biCompression != 0 ||
        (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32)) {
        return String ();
    }

    // convert to image format
    const UInt8* rawBMPPixels = rawBMPInfoHeader + 40;
    if (infoHeader.biBitCount == 24) {
        return doBGRFromIClipboard (
            rawBMPPixels, infoHeader.biWidth, infoHeader.biHeight);
    } else {
        return doBGRAFromIClipboard (
            rawBMPPixels, infoHeader.biWidth, infoHeader.biHeight);
    }
}

String
XWindowsClipboardAnyBitmapConverter::toIClipboard (const String& image) const {
    // convert to raw BMP data
    UInt32 w, h, depth;
    String rawBMP = doToIClipboard (image, w, h, depth);
    if (rawBMP.empty () || w == 0 || h == 0 || (depth != 24 && depth != 32)) {
        return String ();
    }

    // fill BMP info header with little-endian data
    UInt8 infoHeader[40];
    UInt8* dst = infoHeader;
    toLE (dst, static_cast<UInt32> (40));
    toLE (dst, static_cast<SInt32> (w));
    toLE (dst, static_cast<SInt32> (h));
    toLE (dst, static_cast<UInt16> (1));
    toLE (dst, static_cast<UInt16> (depth));
    toLE (dst, static_cast<UInt32> (0)); // BI_RGB
    toLE (dst, static_cast<UInt32> (image.size ()));
    toLE (dst, static_cast<SInt32> (2834)); // 72 dpi
    toLE (dst, static_cast<SInt32> (2834)); // 72 dpi
    toLE (dst, static_cast<UInt32> (0));
    toLE (dst, static_cast<UInt32> (0));

    // construct image
    return String (reinterpret_cast<const char*> (infoHeader),
                   sizeof (infoHeader)) +
           rawBMP;
}

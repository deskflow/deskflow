/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CXWindowsClipboardAnyBitmapConverter.h"

// BMP info header structure
struct CBMPInfoHeader {
public:
	UInt32				biSize;
	SInt32				biWidth;
	SInt32				biHeight;
	UInt16				biPlanes;
	UInt16				biBitCount;
	UInt32				biCompression;
	UInt32				biSizeImage;
	SInt32				biXPelsPerMeter;
	SInt32				biYPelsPerMeter;
	UInt32				biClrUsed;
	UInt32				biClrImportant;
};

// BMP is little-endian
static inline
UInt16
toLE(UInt16 data)
{
	union x16 {
		UInt8	n8[2];
		UInt16	n16;
	} c;
	c.n8[0] = static_cast<UInt8>(data & 0xffu);
	c.n8[1] = static_cast<UInt8>((data >> 8) & 0xffu);
	return c.n16;
}

static inline
SInt32
toLE(SInt32 data)
{
	union x32 {
		UInt8	n8[4];
		SInt32	n32;
	} c;
	c.n8[0] = static_cast<UInt8>(data & 0xffu);
	c.n8[1] = static_cast<UInt8>((data >>  8) & 0xffu);
	c.n8[2] = static_cast<UInt8>((data >> 16) & 0xffu);
	c.n8[3] = static_cast<UInt8>((data >> 24) & 0xffu);
	return c.n32;
}

static inline
UInt32
toLE(UInt32 data)
{
	union x32 {
		UInt8	n8[4];
		UInt32	n32;
	} c;
	c.n8[0] = static_cast<UInt8>(data & 0xffu);
	c.n8[1] = static_cast<UInt8>((data >>  8) & 0xffu);
	c.n8[2] = static_cast<UInt8>((data >> 16) & 0xffu);
	c.n8[3] = static_cast<UInt8>((data >> 24) & 0xffu);
	return c.n32;
}

static inline
UInt16
fromLEU16(const UInt8* data)
{
	return static_cast<UInt16>(data[0]) |
			(static_cast<UInt16>(data[1]) << 8);
}

static inline
SInt32
fromLES32(const UInt8* data)
{
	return static_cast<SInt32>(static_cast<UInt32>(data[0]) |
			(static_cast<UInt32>(data[1]) <<  8) |
			(static_cast<UInt32>(data[2]) << 16) |
			(static_cast<UInt32>(data[3]) << 24));
}

static inline
UInt32
fromLEU32(const UInt8* data)
{
	return static_cast<UInt32>(data[0]) |
			(static_cast<UInt32>(data[1]) <<  8) |
			(static_cast<UInt32>(data[2]) << 16) |
			(static_cast<UInt32>(data[3]) << 24);
}


//
// CXWindowsClipboardAnyBitmapConverter
//

CXWindowsClipboardAnyBitmapConverter::CXWindowsClipboardAnyBitmapConverter()
{
	// do nothing
}

CXWindowsClipboardAnyBitmapConverter::~CXWindowsClipboardAnyBitmapConverter()
{
	// do nothing
}

IClipboard::EFormat
CXWindowsClipboardAnyBitmapConverter::getFormat() const
{
	return IClipboard::kBitmap;
}

int
CXWindowsClipboardAnyBitmapConverter::getDataSize() const
{
	return 8;
}

CString
CXWindowsClipboardAnyBitmapConverter::fromIClipboard(const CString& bmp) const
{
	// fill BMP info header with native-endian data
	CBMPInfoHeader infoHeader;
	const UInt8* rawBMPInfoHeader = reinterpret_cast<const UInt8*>(bmp.data());
	infoHeader.biSize             = fromLEU32(rawBMPInfoHeader +  0);
	infoHeader.biWidth            = fromLES32(rawBMPInfoHeader +  4);
	infoHeader.biHeight           = fromLES32(rawBMPInfoHeader +  8);
	infoHeader.biPlanes           = fromLEU16(rawBMPInfoHeader + 12);
	infoHeader.biBitCount         = fromLEU16(rawBMPInfoHeader + 14);
	infoHeader.biCompression      = fromLEU32(rawBMPInfoHeader + 16);
	infoHeader.biSizeImage        = fromLEU32(rawBMPInfoHeader + 20);
	infoHeader.biXPelsPerMeter    = fromLES32(rawBMPInfoHeader + 24);
	infoHeader.biYPelsPerMeter    = fromLES32(rawBMPInfoHeader + 28);
	infoHeader.biClrUsed          = fromLEU32(rawBMPInfoHeader + 32);
	infoHeader.biClrImportant     = fromLEU32(rawBMPInfoHeader + 36);

	// check that format is acceptable
	if (infoHeader.biSize != 40 ||
		infoHeader.biWidth == 0 || infoHeader.biHeight == 0 ||
		infoHeader.biPlanes != 0 || infoHeader.biCompression != 0 ||
		(infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32)) {
		return CString();
	}

	// convert to image format
	const UInt8* rawBMPPixels = rawBMPInfoHeader + 40;
	if (infoHeader.biBitCount == 24) {
		return doBGRFromIClipboard(rawBMPPixels,
							infoHeader.biWidth, infoHeader.biHeight);
	}
	else {
		return doBGRAFromIClipboard(rawBMPPixels,
							infoHeader.biWidth, infoHeader.biHeight);
	}
}

CString
CXWindowsClipboardAnyBitmapConverter::toIClipboard(const CString& image) const
{
	// convert to raw BMP data
	UInt32 w, h, depth;
	CString rawBMP = doToIClipboard(image, w, h, depth);
	if (rawBMP.empty() || w == 0 || h == 0 || (depth != 24 && depth != 32)) {
		return CString();
	}

	// fill BMP info header with little-endian data
	CBMPInfoHeader infoHeader;
	infoHeader.biSize          = toLE(40);
	infoHeader.biWidth         = toLE(w);
	infoHeader.biHeight        = toLE(h);
	infoHeader.biPlanes        = toLE(1);
	infoHeader.biBitCount      = toLE(depth);
	infoHeader.biCompression   = toLE(0);		// BI_RGB
	infoHeader.biSizeImage     = image.size();
	infoHeader.biXPelsPerMeter = toLE(2834);	// 72 dpi
	infoHeader.biYPelsPerMeter = toLE(2834);	// 72 dpi
	infoHeader.biClrUsed       = toLE(0);
	infoHeader.biClrImportant  = toLE(0);

	// construct image
	return CString(reinterpret_cast<const char*>(&infoHeader),
							sizeof(infoHeader)) + rawBMP;
}

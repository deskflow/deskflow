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

#include "CXWindowsClipboardBMPConverter.h"

// BMP file header structure
struct CBMPHeader {
public:
	UInt16				type;
	UInt32				size;
	UInt16				reserved1;
	UInt16				reserved2;
	UInt32				offset;
};

// BMP is little-endian
static inline
UInt32
fromLEU32(const UInt8* data)
{
	return static_cast<UInt32>(data[0]) |
			(static_cast<UInt32>(data[1]) <<  8) |
			(static_cast<UInt32>(data[2]) << 16) |
			(static_cast<UInt32>(data[3]) << 24);
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

//
// CXWindowsClipboardBMPConverter
//

CXWindowsClipboardBMPConverter::CXWindowsClipboardBMPConverter(
				Display* display) :
	m_atom(XInternAtom(display, "image/bmp", False))
{
	// do nothing
}

CXWindowsClipboardBMPConverter::~CXWindowsClipboardBMPConverter()
{
	// do nothing
}

IClipboard::EFormat
CXWindowsClipboardBMPConverter::getFormat() const
{
	return IClipboard::kBitmap;
}

Atom
CXWindowsClipboardBMPConverter::getAtom() const
{
	return m_atom;
}

int
CXWindowsClipboardBMPConverter::getDataSize() const
{
	return 8;
}

CString
CXWindowsClipboardBMPConverter::fromIClipboard(const CString& bmp) const
{
	// create BMP image
	CBMPHeader header;
	char* type       = reinterpret_cast<char*>(&header.type);
	type[0]          = 'B';
	type[1]          = 'M';
	header.size      = toLE(14 + bmp.size());
	header.reserved1 = 0;
	header.reserved2 = 0;
	header.offset    = toLE(14 + 40);
	return CString(reinterpret_cast<const char*>(&header), 14) + bmp;
}

CString
CXWindowsClipboardBMPConverter::toIClipboard(const CString& bmp) const
{
	// make sure data is big enough for a BMP file
	if (bmp.size() <= 14 + 40) {
		return CString();
	}

	// check BMP file header
	const UInt8* rawBMPHeader = reinterpret_cast<const UInt8*>(bmp.data());
	if (rawBMPHeader[0] != 'B' || rawBMPHeader[1] != 'M') {
		return CString();
	}

	// get offset to image data
	UInt32 offset = fromLEU32(rawBMPHeader + 10);

	// construct BMP
	if (offset == 14 + 40) {
		return bmp.substr(14);
	}
	else {
		return bmp.substr(14, 40) + bmp.substr(offset, bmp.size() - offset);
	}
}

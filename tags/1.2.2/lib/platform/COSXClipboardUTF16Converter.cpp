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

#include "COSXClipboardUTF16Converter.h"
#include "CUnicode.h"

//
// COSXClipboardUTF16Converter
//

COSXClipboardUTF16Converter::COSXClipboardUTF16Converter()
{
	// do nothing
}

COSXClipboardUTF16Converter::~COSXClipboardUTF16Converter()
{
	// do nothing
}

ScrapFlavorType
COSXClipboardUTF16Converter::getOSXFormat() const
{
	return kScrapFlavorTypeUnicode;
}

CString
COSXClipboardUTF16Converter::doFromIClipboard(const CString& data) const
{
	// convert and add nul terminator
	return CUnicode::UTF8ToUTF16(data);
}

CString
COSXClipboardUTF16Converter::doToIClipboard(const CString& data) const
{
	// convert and strip nul terminator
	return CUnicode::UTF16ToUTF8(data);
}

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

#include "COSXClipboardTextConverter.h"
#include "CUnicode.h"

//
// COSXClipboardTextConverter
//

COSXClipboardTextConverter::COSXClipboardTextConverter() 
{
	// do nothing
}

COSXClipboardTextConverter::~COSXClipboardTextConverter()
{
	// do nothing
}

ScrapFlavorType
COSXClipboardTextConverter::getOSXFormat() const
{
	return kScrapFlavorTypeText;
}

CString
COSXClipboardTextConverter::doFromIClipboard(const CString& data) const
{
	return CUnicode::UTF8ToText(data);
}

CString
COSXClipboardTextConverter::doToIClipboard(const CString& data) const
{
	return CUnicode::textToUTF8(data);
}

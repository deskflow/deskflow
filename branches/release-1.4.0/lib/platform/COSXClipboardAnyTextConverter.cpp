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

#include "COSXClipboardAnyTextConverter.h"
#include <algorithm>

//
// COSXClipboardAnyTextConverter
//

COSXClipboardAnyTextConverter::COSXClipboardAnyTextConverter()
{
	// do nothing
}

COSXClipboardAnyTextConverter::~COSXClipboardAnyTextConverter()
{
	// do nothing
}

IClipboard::EFormat
COSXClipboardAnyTextConverter::getFormat() const
{
	return IClipboard::kText;
}

CString
COSXClipboardAnyTextConverter::fromIClipboard(const CString& data) const
{
	// convert linefeeds and then convert to desired encoding
	return doFromIClipboard(convertLinefeedToMacOS(data));
}

CString
COSXClipboardAnyTextConverter::toIClipboard(const CString& data) const
{
	// convert text then newlines
	return convertLinefeedToUnix(doToIClipboard(data));
}

static
bool
isLF(char ch)
{
    return (ch == '\n');
}

static
bool
isCR(char ch)
{
    return (ch == '\r');
}

CString
COSXClipboardAnyTextConverter::convertLinefeedToMacOS(const CString& src)
{
	// note -- we assume src is a valid UTF-8 string
    CString copy = src;

    std::replace_if(copy.begin(), copy.end(), isLF, '\r');

	return copy;
}

CString
COSXClipboardAnyTextConverter::convertLinefeedToUnix(const CString& src)
{
    CString copy = src;

    std::replace_if(copy.begin(), copy.end(), isCR, '\n');

	return copy;
}

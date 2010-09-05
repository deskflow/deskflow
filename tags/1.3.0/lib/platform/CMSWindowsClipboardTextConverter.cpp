/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
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

#include "CMSWindowsClipboardTextConverter.h"
#include "CUnicode.h"

//
// CMSWindowsClipboardTextConverter
//

CMSWindowsClipboardTextConverter::CMSWindowsClipboardTextConverter()
{
	// do nothing
}

CMSWindowsClipboardTextConverter::~CMSWindowsClipboardTextConverter()
{
	// do nothing
}

UINT
CMSWindowsClipboardTextConverter::getWin32Format() const
{
	return CF_TEXT;
}

CString
CMSWindowsClipboardTextConverter::doFromIClipboard(const CString& data) const
{
	// convert and add nul terminator
	return CUnicode::UTF8ToText(data) += '\0';
}

CString
CMSWindowsClipboardTextConverter::doToIClipboard(const CString& data) const
{
	// convert and truncate at first nul terminator
	CString dst          = CUnicode::textToUTF8(data);
	CString::size_type n = dst.find('\0');
	if (n != CString::npos) {
		dst.erase(n);
	}
	return dst;
}

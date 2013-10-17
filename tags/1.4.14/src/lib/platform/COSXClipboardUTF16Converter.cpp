/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

CFStringRef
COSXClipboardUTF16Converter::getOSXFormat() const
{
	return CFSTR("public.utf16-plain-text");
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

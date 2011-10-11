/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CMSWindowsClipboardUTF16Converter.h"
#include "CUnicode.h"

//
// CMSWindowsClipboardUTF16Converter
//

CMSWindowsClipboardUTF16Converter::CMSWindowsClipboardUTF16Converter()
{
	// do nothing
}

CMSWindowsClipboardUTF16Converter::~CMSWindowsClipboardUTF16Converter()
{
	// do nothing
}

UINT
CMSWindowsClipboardUTF16Converter::getWin32Format() const
{
	return CF_UNICODETEXT;
}

CString
CMSWindowsClipboardUTF16Converter::doFromIClipboard(const CString& data) const
{
	// convert and add nul terminator
	return CUnicode::UTF8ToUTF16(data).append(sizeof(wchar_t), 0);
}

CString
CMSWindowsClipboardUTF16Converter::doToIClipboard(const CString& data) const
{
	// convert and strip nul terminator
	CString dst          = CUnicode::UTF16ToUTF8(data);
	CString::size_type n = dst.find('\0');
	if (n != CString::npos) {
		dst.erase(n);
	}
	return dst;
}

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

#include "CMSWindowsClipboardHTMLConverter.h"
#include "CStringUtil.h"

//
// CMSWindowsClipboardHTMLConverter
//

CMSWindowsClipboardHTMLConverter::CMSWindowsClipboardHTMLConverter()
{
	m_format = RegisterClipboardFormat("HTML Format");
}

CMSWindowsClipboardHTMLConverter::~CMSWindowsClipboardHTMLConverter()
{
	// do nothing
}

IClipboard::EFormat
CMSWindowsClipboardHTMLConverter::getFormat() const
{
	return IClipboard::kHTML;
}

UINT
CMSWindowsClipboardHTMLConverter::getWin32Format() const
{
	return m_format;
}

CString
CMSWindowsClipboardHTMLConverter::doFromIClipboard(const CString& data) const
{
	// prepare to CF_HTML format prefix and suffix
	CString prefix("Version:0.9\r\nStartHTML:0000000105\r\n"
					"EndHTML:ZZZZZZZZZZ\r\n"
					"StartFragment:XXXXXXXXXX\r\nEndFragment:YYYYYYYYYY\r\n"
					"<!DOCTYPE><HTML><BODY><!--StartFragment-->");
	CString suffix("<!--EndFragment--></BODY></HTML>\r\n");

	// Get byte offsets for header
	UInt32 StartFragment = (UInt32)prefix.size();
	UInt32 EndFragment   = StartFragment + (UInt32)data.size();
	// StartHTML is constant by the design of the prefix
	UInt32 EndHTML = EndFragment + (UInt32)suffix.size();

	prefix.replace(prefix.find("XXXXXXXXXX"), 10,
							CStringUtil::print("%010u", StartFragment));
	prefix.replace(prefix.find("YYYYYYYYYY"), 10,
							CStringUtil::print("%010u", EndFragment));
	prefix.replace(prefix.find("ZZZZZZZZZZ"), 10,
							CStringUtil::print("%010u", EndHTML));

	// concatenate
	prefix += data;
	prefix += suffix;
	return prefix;
}

CString
CMSWindowsClipboardHTMLConverter::doToIClipboard(const CString& data) const
{
	// get fragment start/end args
	CString startArg = findArg(data, "StartFragment");
	CString endArg   = findArg(data, "EndFragment");
	if (startArg.empty() || endArg.empty()) {
		return CString();
	}

	// convert args to integers
	SInt32 start = (SInt32)atoi(startArg.c_str());
	SInt32 end   = (SInt32)atoi(endArg.c_str());
	if (start <= 0 || end <= 0 || start >= end) {
		return CString();
	}

	// extract the fragment
	return data.substr(start, end - start);
}

CString
CMSWindowsClipboardHTMLConverter::findArg(
				const CString& data, const CString& name) const
{
	CString::size_type i = data.find(name);
	if (i == CString::npos) {
		return CString();
	}
	i = data.find_first_of(":\r\n", i);
	if (i == CString::npos || data[i] != ':') {
		return CString();
	}
	i = data.find_first_of("0123456789\r\n", i + 1);
	if (i == CString::npos || data[i] == '\r' || data[i] == '\n') {
		return CString();
	}
	CString::size_type j = data.find_first_not_of("0123456789", i);
	if (j == CString::npos) {
		j = data.size();
	}
	return data.substr(i, j - i);
}

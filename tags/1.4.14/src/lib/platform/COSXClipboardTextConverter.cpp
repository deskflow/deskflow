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

CFStringRef
COSXClipboardTextConverter::getOSXFormat() const
{
	return CFSTR("public.plain-text");
}

CString 
COSXClipboardTextConverter::convertString(
				const CString& data, 
				CFStringEncoding fromEncoding,
				CFStringEncoding toEncoding)
{
	CFStringRef stringRef =
		CFStringCreateWithCString(kCFAllocatorDefault,
							data.c_str(), fromEncoding);

	if (stringRef == NULL) {
		return CString();
	}

	CFIndex buffSize;
	CFRange entireString = CFRangeMake(0, CFStringGetLength(stringRef));

	CFStringGetBytes(stringRef, entireString, toEncoding,
							0, false, NULL, 0, &buffSize);

	char* buffer = new char[buffSize];
	
	if (buffer == NULL) {
		CFRelease(stringRef);
		return CString();
	}
	
	CFStringGetBytes(stringRef, entireString, toEncoding,
							0, false, (UInt8*)buffer, buffSize, NULL);

	CString result(buffer, buffSize);

	delete[] buffer;
	CFRelease(stringRef);
	
	return result;
}

CString
COSXClipboardTextConverter::doFromIClipboard(const CString& data) const
{
	return convertString(data, kCFStringEncodingUTF8,
							CFStringGetSystemEncoding());
}

CString
COSXClipboardTextConverter::doToIClipboard(const CString& data) const
{
	return convertString(data, CFStringGetSystemEncoding(),
							kCFStringEncodingUTF8);
}

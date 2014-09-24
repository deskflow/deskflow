/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
 * Patch by Ryan Chapman
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

#pragma once

#include "OSXClipboardAnyTextConverter.h"

//! Convert to/from HTML encoding
class COSXClipboardHTMLConverter : public COSXClipboardAnyTextConverter {
public:
	COSXClipboardHTMLConverter();
	virtual ~COSXClipboardHTMLConverter();

	// IMSWindowsClipboardConverter overrides
	virtual IClipboard::EFormat
						getFormat() const;

	virtual CFStringRef	getOSXFormat() const;

protected:
	// COSXClipboardAnyTextConverter overrides
	virtual CString		doFromIClipboard(const CString&) const;
	virtual CString		doToIClipboard(const CString&) const;

	// generic encoding converter
	static CString		convertString(const CString& data,
							CFStringEncoding fromEncoding,
							CFStringEncoding toEncoding);
};

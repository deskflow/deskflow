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

#ifndef COSXCLIPBOARDTEXTCONVERTER_H
#define COSXCLIPBOARDTEXTCONVERTER_H

#include "COSXClipboardAnyTextConverter.h"

//! Convert to/from locale text encoding
class COSXClipboardTextConverter : public COSXClipboardAnyTextConverter {
public:
	COSXClipboardTextConverter();
	virtual ~COSXClipboardTextConverter();

	// IOSXClipboardAnyTextConverter overrides
	virtual CFStringRef
						getOSXFormat() const;

protected:
	// COSXClipboardAnyTextConverter overrides
	virtual CString		doFromIClipboard(const CString&) const;
	virtual CString		doToIClipboard(const CString&) const;

	// generic encoding converter
	static CString		convertString(const CString& data,  
							CFStringEncoding fromEncoding,
							CFStringEncoding toEncoding);
};

#endif

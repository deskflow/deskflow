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

#ifndef COSXCLIPBOARDANYTEXTCONVERTER_H
#define COSXCLIPBOARDANYTEXTCONVERTER_H

#include "COSXClipboard.h"

//! Convert to/from some text encoding
class COSXClipboardAnyTextConverter : public IOSXClipboardConverter {
public:
	COSXClipboardAnyTextConverter();
	virtual ~COSXClipboardAnyTextConverter();

	// IOSXClipboardConverter overrides
	virtual IClipboard::EFormat
						getFormat() const;
	virtual ScrapFlavorType
						getOSXFormat() const = 0;
	virtual CString		fromIClipboard(const CString &) const;
	virtual CString		toIClipboard(const CString &) const;

protected:
	//! Convert from IClipboard format
	/*!
	Do UTF-8 conversion and linefeed conversion.
	*/
	virtual CString		doFromIClipboard(const CString&) const = 0;

	//! Convert to IClipboard format
	/*!
	Do UTF-8 conversion and Linefeed conversion.
	*/
	virtual CString		doToIClipboard(const CString&) const = 0;

private:
	static CString		convertLinefeedToMacOS(const CString&);
	static CString		convertLinefeedToUnix(const CString&);
};

#endif

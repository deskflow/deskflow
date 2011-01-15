/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CXWINDOWSCLIPBOARDANYBITMAPCONVERTER_H
#define CXWINDOWSCLIPBOARDANYBITMAPCONVERTER_H

#include "CXWindowsClipboard.h"

//! Convert to/from some text encoding
class CXWindowsClipboardAnyBitmapConverter :
				public IXWindowsClipboardConverter {
public:
	CXWindowsClipboardAnyBitmapConverter();
	virtual ~CXWindowsClipboardAnyBitmapConverter();

	// IXWindowsClipboardConverter overrides
	virtual IClipboard::EFormat
						getFormat() const;
	virtual Atom		getAtom() const = 0;
	virtual int			getDataSize() const;
	virtual CString		fromIClipboard(const CString&) const;
	virtual CString		toIClipboard(const CString&) const;

protected:
	//! Convert from IClipboard format
	/*!
	Convert raw BGR pixel data to another image format.
	*/
	virtual CString		doBGRFromIClipboard(const UInt8* bgrData,
							UInt32 w, UInt32 h) const = 0;

	//! Convert from IClipboard format
	/*!
	Convert raw BGRA pixel data to another image format.
	*/
	virtual CString		doBGRAFromIClipboard(const UInt8* bgrData,
							UInt32 w, UInt32 h) const = 0;

	//! Convert to IClipboard format
	/*!
	Convert an image into raw BGR or BGRA image data and store the
	width, height, and image depth (24 or 32).
	*/
	virtual CString		doToIClipboard(const CString&,
							UInt32& w, UInt32& h, UInt32& depth) const = 0;
};

#endif

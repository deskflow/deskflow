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

#ifndef CXWINDOWSCLIPBOARDUCS2CONVERTER_H
#define CXWINDOWSCLIPBOARDUCS2CONVERTER_H

#include "CXWindowsClipboard.h"

//! Convert to/from UCS-2 encoding
class CXWindowsClipboardUCS2Converter : public IXWindowsClipboardConverter {
public:
	/*!
	\c name is converted to an atom and that is reported by getAtom().
	*/
	CXWindowsClipboardUCS2Converter(Display* display, const char* name);
	virtual ~CXWindowsClipboardUCS2Converter();

	// IXWindowsClipboardConverter overrides
	virtual IClipboard::EFormat
						getFormat() const;
	virtual Atom		getAtom() const;
	virtual int			getDataSize() const;
	virtual CString		fromIClipboard(const CString&) const;
	virtual CString		toIClipboard(const CString&) const;

private:
	Atom				m_atom;
};

#endif

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

#ifndef CMSWINDOWSCLIPBOARDBITMAPCONVERTER_H
#define CMSWINDOWSCLIPBOARDBITMAPCONVERTER_H

#include "CMSWindowsClipboard.h"

//! Convert to/from some text encoding
class CMSWindowsClipboardBitmapConverter :
				public IMSWindowsClipboardConverter {
public:
	CMSWindowsClipboardBitmapConverter();
	virtual ~CMSWindowsClipboardBitmapConverter();

	// IMSWindowsClipboardConverter overrides
	virtual IClipboard::EFormat
						getFormat() const;
	virtual UINT		getWin32Format() const;
	virtual HANDLE		fromIClipboard(const CString&) const;
	virtual CString		toIClipboard(HANDLE) const;
};

#endif

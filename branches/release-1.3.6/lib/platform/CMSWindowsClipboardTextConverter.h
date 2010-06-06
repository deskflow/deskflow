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

#ifndef CMSWINDOWSCLIPBOARDTEXTCONVERTER_H
#define CMSWINDOWSCLIPBOARDTEXTCONVERTER_H

#include "CMSWindowsClipboardAnyTextConverter.h"

//! Convert to/from locale text encoding
class CMSWindowsClipboardTextConverter :
				public CMSWindowsClipboardAnyTextConverter {
public:
	CMSWindowsClipboardTextConverter();
	virtual ~CMSWindowsClipboardTextConverter();

	// IMSWindowsClipboardConverter overrides
	virtual UINT		getWin32Format() const;

protected:
	// CMSWindowsClipboardAnyTextConverter overrides
	virtual CString		doFromIClipboard(const CString&) const;
	virtual CString		doToIClipboard(const CString&) const;
};

#endif

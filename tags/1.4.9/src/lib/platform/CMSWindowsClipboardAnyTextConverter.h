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

#ifndef CMSWINDOWSCLIPBOARDANYTEXTCONVERTER_H
#define CMSWINDOWSCLIPBOARDANYTEXTCONVERTER_H

#include "CMSWindowsClipboard.h"

//! Convert to/from some text encoding
class CMSWindowsClipboardAnyTextConverter :
				public IMSWindowsClipboardConverter {
public:
	CMSWindowsClipboardAnyTextConverter();
	virtual ~CMSWindowsClipboardAnyTextConverter();

	// IMSWindowsClipboardConverter overrides
	virtual IClipboard::EFormat
						getFormat() const;
	virtual UINT		getWin32Format() const = 0;
	virtual HANDLE		fromIClipboard(const CString&) const;
	virtual CString		toIClipboard(HANDLE) const;

protected:
	//! Convert from IClipboard format
	/*!
	Do UTF-8 conversion only.  Memory handle allocation and
	linefeed conversion is done by this class.  doFromIClipboard()
	must include the nul terminator in the returned string (not
	including the CString's nul terminator).
	*/
	virtual CString		doFromIClipboard(const CString&) const = 0;

	//! Convert to IClipboard format
	/*!
	Do UTF-8 conversion only.  Memory handle allocation and
	linefeed conversion is done by this class.
	*/
	virtual CString		doToIClipboard(const CString&) const = 0;

private:
	CString				convertLinefeedToWin32(const CString&) const;
	CString				convertLinefeedToUnix(const CString&) const;
};

#endif

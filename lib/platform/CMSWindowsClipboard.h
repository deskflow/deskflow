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

#ifndef CMSWINDOWSCLIPBOARD_H
#define CMSWINDOWSCLIPBOARD_H

#include "IClipboard.h"
#include "stdvector.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class IMSWindowsClipboardConverter;

//! Microsoft windows clipboard implementation
class CMSWindowsClipboard : public IClipboard {
public:
	CMSWindowsClipboard(HWND window);
	virtual ~CMSWindowsClipboard();

	// IClipboard overrides
	virtual bool		empty();
	virtual void		add(EFormat, const CString& data);
	virtual bool		open(Time) const;
	virtual void		close() const;
	virtual Time		getTime() const;
	virtual bool		has(EFormat) const;
	virtual CString		get(EFormat) const;

private:
	void				clearConverters();

	UINT				convertFormatToWin32(EFormat) const;
	HANDLE				convertTextToWin32(const CString& data) const;
	CString				convertTextFromWin32(HANDLE) const;

private:
	typedef std::vector<IMSWindowsClipboardConverter*> ConverterList;

	HWND				m_window;
	mutable Time		m_time;
	ConverterList		m_converters;
};

//! Clipboard format converter interface
/*!
This interface defines the methods common to all win32 clipboard format
converters.
*/
class IMSWindowsClipboardConverter : public IInterface {
public:
	// accessors

	// return the clipboard format this object converts from/to
	virtual IClipboard::EFormat
						getFormat() const = 0;

	// return the atom representing the win32 clipboard format that
	// this object converts from/to
	virtual UINT		getWin32Format() const = 0;

	// convert from the IClipboard format to the win32 clipboard format.
	// the input data must be in the IClipboard format returned by
	// getFormat().  the return data will be in the win32 clipboard
	// format returned by getWin32Format(), allocated by GlobalAlloc().
	virtual HANDLE		fromIClipboard(const CString&) const = 0;

	// convert from the win32 clipboard format to the IClipboard format
	// (i.e., the reverse of fromIClipboard()).
	virtual CString		toIClipboard(HANDLE data) const = 0;
};

#endif

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

#include "CMSWindowsClipboard.h"
#include "CMSWindowsClipboardTextConverter.h"
#include "CMSWindowsClipboardUTF16Converter.h"
#include "CLog.h"
#include "CArchMiscWindows.h"

//
// CMSWindowsClipboard
//

UINT					CMSWindowsClipboard::s_ownershipFormat = 0;

CMSWindowsClipboard::CMSWindowsClipboard(HWND window) :
	m_window(window),
	m_time(0)
{
	// add converters, most desired first
	m_converters.push_back(new CMSWindowsClipboardUTF16Converter);
	if (CArchMiscWindows::isWindows95Family()) {
		// windows nt family converts to/from unicode automatically.
		// let it do so to avoid text encoding issues.
		m_converters.push_back(new CMSWindowsClipboardTextConverter);
	}
}

CMSWindowsClipboard::~CMSWindowsClipboard()
{
	clearConverters();
}

bool
CMSWindowsClipboard::emptyUnowned()
{
	LOG((CLOG_DEBUG "empty clipboard"));

	// empty the clipboard (and take ownership)
	if (!EmptyClipboard()) {
		LOG((CLOG_DEBUG "failed to grab clipboard"));
		return false;
	}

	return true;
}

bool
CMSWindowsClipboard::empty()
{
	if (!emptyUnowned()) {
		return false;
	}

	// mark clipboard as being owned by synergy
	HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, 1);
	SetClipboardData(getOwnershipFormat(), data);

	return true;
}

void
CMSWindowsClipboard::add(EFormat format, const CString& data)
{
	LOG((CLOG_DEBUG "add %d bytes to clipboard format: %d", data.size(), format));

	// convert data to win32 form
	for (ConverterList::const_iterator index = m_converters.begin();
								index != m_converters.end(); ++index) {
		IMSWindowsClipboardConverter* converter = *index;

		// skip converters for other formats
		if (converter->getFormat() == format) {
			HANDLE win32Data = converter->fromIClipboard(data);
			if (win32Data != NULL) {
				UINT win32Format = converter->getWin32Format();
				if (SetClipboardData(win32Format, win32Data) == NULL) {
					// free converted data if we couldn't put it on
					// the clipboard
					GlobalFree(win32Data);
				}
			}
		}
	}
}

bool
CMSWindowsClipboard::open(Time time) const
{
	LOG((CLOG_DEBUG "open clipboard"));

	if (!OpenClipboard(m_window)) {
		LOG((CLOG_WARN "failed to open clipboard"));
		return false;
	}

	m_time = time;

	return true;
}

void
CMSWindowsClipboard::close() const
{
	LOG((CLOG_DEBUG "close clipboard"));
	CloseClipboard();
}

IClipboard::Time
CMSWindowsClipboard::getTime() const
{
	return m_time;
}

bool
CMSWindowsClipboard::has(EFormat format) const
{
	for (ConverterList::const_iterator index = m_converters.begin();
								index != m_converters.end(); ++index) {
		IMSWindowsClipboardConverter* converter = *index;
		if (converter->getFormat() == format) {
			if (IsClipboardFormatAvailable(converter->getWin32Format())) {
				return true;
			}
		}
	}
	return false;
}

CString
CMSWindowsClipboard::get(EFormat format) const
{
	// find the converter for the first clipboard format we can handle
	IMSWindowsClipboardConverter* converter = NULL;
	UINT win32Format = EnumClipboardFormats(0);
	while (converter == NULL && win32Format != 0) {
		for (ConverterList::const_iterator index = m_converters.begin();
								index != m_converters.end(); ++index) {
			converter = *index;
			if (converter->getWin32Format() == win32Format &&
				converter->getFormat()      == format) {
				break;
			}
			converter = NULL;
		}
		win32Format = EnumClipboardFormats(win32Format);
	}

	// if no converter then we don't recognize any formats
	if (converter == NULL) {
		return CString();
	}

	// get a handle to the clipboard data
	HANDLE win32Data = GetClipboardData(converter->getWin32Format());
	if (win32Data == NULL) {
		return CString();
	}

	// convert
	return converter->toIClipboard(win32Data);
}

void
CMSWindowsClipboard::clearConverters()
{
	for (ConverterList::iterator index = m_converters.begin();
								index != m_converters.end(); ++index) {
		delete *index;
	}
	m_converters.clear();
}

bool
CMSWindowsClipboard::isOwnedBySynergy()
{
	// create ownership format if we haven't yet
	if (s_ownershipFormat == 0) {
		s_ownershipFormat = RegisterClipboardFormat(TEXT("SynergyOwnership"));
	}
	return (IsClipboardFormatAvailable(getOwnershipFormat()) != 0);
}

UINT
CMSWindowsClipboard::getOwnershipFormat()
{
	// create ownership format if we haven't yet
	if (s_ownershipFormat == 0) {
		s_ownershipFormat = RegisterClipboardFormat(TEXT("SynergyOwnership"));
	}

	// return the format
	return s_ownershipFormat;
}

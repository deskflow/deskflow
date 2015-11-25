/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/MSWindowsClipboard.h"

#include "platform/MSWindowsClipboardTextConverter.h"
#include "platform/MSWindowsClipboardUTF16Converter.h"
#include "platform/MSWindowsClipboardBitmapConverter.h"
#include "platform/MSWindowsClipboardHTMLConverter.h"
#include "platform/MSWindowsClipboardFacade.h"
#include "arch/win32/ArchMiscWindows.h"
#include "base/Log.h"

//
// MSWindowsClipboard
//

UINT					MSWindowsClipboard::s_ownershipFormat = 0;

MSWindowsClipboard::MSWindowsClipboard(HWND window) :
	m_window(window),
	m_time(0),
	m_facade(new MSWindowsClipboardFacade()),
	m_deleteFacade(true)
{
	LOG((CLOG_DEBUG "%s", __FUNCTION__));

	// add converters, most desired first
	m_converters.push_back(new MSWindowsClipboardUTF16Converter);
	m_converters.push_back(new MSWindowsClipboardBitmapConverter);
	m_converters.push_back(new MSWindowsClipboardHTMLConverter);
}

MSWindowsClipboard::~MSWindowsClipboard()
{
	LOG((CLOG_DEBUG "%s", __FUNCTION__));

	clearConverters();

	// dependency injection causes confusion over ownership, so we need
	// logic to decide whether or not we delete the facade. there must
	// be a more elegant way of doing this.
	if (m_deleteFacade)
		delete m_facade;
}

void
MSWindowsClipboard::setFacade(IMSWindowsClipboardFacade& facade)
{
	delete m_facade;
	m_facade = &facade;
	m_deleteFacade = false;
}

bool
MSWindowsClipboard::emptyUnowned()
{
	LOG((CLOG_DEBUG "empty clipboard"));

	// empty the clipboard (and take ownership)
	if (!EmptyClipboard()) {
		// unable to cause this in integ tests, but this error has never 
		// actually been reported by users.
		LOG((CLOG_DEBUG "failed to grab clipboard"));
		return false;
	}

	return true;
}

bool
MSWindowsClipboard::empty()
{
	LOG((CLOG_DEBUG "%s", __FUNCTION__));

	if (!emptyUnowned()) {
		return false;
	}

	// mark clipboard as being owned by synergy
	HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, 1);
	SetClipboardData(getOwnershipFormat(), data);

	return true;
}

void
MSWindowsClipboard::add(EFormat format, const String& data)
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
				m_facade->write(win32Data, win32Format);
			}
		}
	}
}

bool
MSWindowsClipboard::open(Time time) const
{
	LOG((CLOG_DEBUG "open clipboard"));

	if (!OpenClipboard(m_window)) {
		// unable to cause this in integ tests; but this can happen!
		// * http://synergy-project.org/pm/issues/86
		// * http://synergy-project.org/pm/issues/1256
		// logging improved to see if we can catch more info next time.
		LOG((CLOG_WARN "failed to open clipboard: %d", GetLastError()));
		return false;
	}

	m_time = time;

	return true;
}

void
MSWindowsClipboard::close() const
{
	LOG((CLOG_DEBUG "close clipboard"));
	CloseClipboard();
}

IClipboard::Time
MSWindowsClipboard::getTime() const
{
	return m_time;
}

bool
MSWindowsClipboard::has(EFormat format) const
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

String
MSWindowsClipboard::get(EFormat format) const
{
	LOG((CLOG_DEBUG "get format %d", format));

	auto l_name = [] (UINT fmt) -> String {
		char buf[100]="";
		if (fmt>=0xc000 && fmt<=0xffff) {
			buf[0]=':';
			GetClipboardFormatName(fmt, buf+1, 99);
			return String(buf);
		}
		return String("");
	};
	auto l_err = [] (DWORD error) -> String {
		if (error==0) return String("");
		LPVOID ptr=NULL;
		DWORD len=FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&ptr,
			0, NULL );
		String ret("");
		if (ptr!=NULL) {
			if (len>0) ret=String((LPSTR)ptr, (LPSTR)ptr+len);
			LocalFree(ptr);
		}
		return ret;
	};
	String s("converters:");
	for (ConverterList::const_iterator index = m_converters.begin();
							index != m_converters.end(); ++index) {
		s+=synergy::string::sprintf(" %d,0x%x%s",
			(*index)->getFormat(),
			(*index)->getWin32Format(),
			l_name((*index)->getWin32Format()).c_str());
	}
	LOG((CLOG_DEBUG "%s\n", s.c_str()));

	s = String("formats:");
	for (UINT win32Format = EnumClipboardFormats(0);
		win32Format != 0; win32Format = EnumClipboardFormats(win32Format)) {
		s+=synergy::string::sprintf(" 0x%x%s",
			win32Format,
			l_name(win32Format).c_str());
	}
	LOG((CLOG_DEBUG "%s %s\n", s.c_str(), l_err(GetLastError()).c_str()));

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

	// if no converter then EnumClipboardFormats() is broken: try just
	// GetClipboardData() directly (Issue $5041)
	if (converter == NULL) {
		LOG((CLOG_INFO "Broken EnumClipboardFormats, falling back to using GetClipboardData"));

		for (ConverterList::const_iterator index = m_converters.begin();
								index != m_converters.end(); ++index) {
			converter = *index;
			if (converter->getFormat() == format) {
				LOG((CLOG_DEBUG "using converter 0x%x%s for %d\n",
					converter->getWin32Format(),
					l_name(converter->getWin32Format()).c_str(),
					format));
				HANDLE win32Data = GetClipboardData(converter->getWin32Format());
				if (win32Data != NULL)
					return converter->toIClipboard(win32Data);
			}
		}
		return String();
	}

	LOG((CLOG_DEBUG "using converter 0x%x%s for %d\n",
		converter->getWin32Format(),
		l_name(converter->getWin32Format()).c_str(),
		format));

	// get a handle to the clipboard data
	HANDLE win32Data = GetClipboardData(converter->getWin32Format());
	if (win32Data == NULL) {
		// nb: can't cause this using integ tests; this is only caused when
		// the selected converter returns an invalid format -- which you
		// cannot cause using public functions.
		LOG((CLOG_WARN "No data for format %d -> win32 format 0x%x",
			format, converter->getWin32Format()));
		return String();
	}

	// convert
	return converter->toIClipboard(win32Data);
}

void
MSWindowsClipboard::clearConverters()
{
	LOG((CLOG_DEBUG "%s", __FUNCTION__));

	for (ConverterList::iterator index = m_converters.begin();
								index != m_converters.end(); ++index) {
		delete *index;
	}
	m_converters.clear();
}

bool
MSWindowsClipboard::isOwnedBySynergy()
{
	// create ownership format if we haven't yet
	if (s_ownershipFormat == 0) {
		s_ownershipFormat = RegisterClipboardFormat(TEXT("SynergyOwnership"));
		LOG((CLOG_DEBUG "%s set ownership %d", __FUNCTION__, s_ownershipFormat));
	}
	return (IsClipboardFormatAvailable(getOwnershipFormat()) != 0);
}

UINT
MSWindowsClipboard::getOwnershipFormat()
{
	// create ownership format if we haven't yet
	if (s_ownershipFormat == 0) {
		s_ownershipFormat = RegisterClipboardFormat(TEXT("SynergyOwnership"));
		LOG((CLOG_DEBUG "%s set ownership", __FUNCTION__));
	}

	LOG((CLOG_DEBUG "%s=%d", __FUNCTION__, s_ownershipFormat));

	// return the format
	return s_ownershipFormat;
}

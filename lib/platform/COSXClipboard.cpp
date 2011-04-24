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

#include "CClipboard.h"
#include "COSXClipboard.h"
#include "COSXClipboardUTF16Converter.h"
#include "COSXClipboardTextConverter.h"
#include "CLog.h"
#include "XArch.h"

//
// COSXClipboard
//

COSXClipboard::COSXClipboard() :
	m_time(0),
	m_pboard(NULL)
{
	m_converters.push_back(new COSXClipboardUTF16Converter);
	m_converters.push_back(new COSXClipboardTextConverter);

	OSStatus createErr = PasteboardCreate(kPasteboardClipboard, &m_pboard);
	if (createErr != noErr) {
		LOG((CLOG_DEBUG "failed to create clipboard reference: error %i", createErr));
		LOG((CLOG_ERR "Unable to connect to pasteboard. Clipboard sharing disabled.", createErr));
		m_pboard = NULL;
		return;

	}

	OSStatus syncErr = PasteboardSynchronize(m_pboard);
	if (syncErr != noErr) {
		LOG((CLOG_DEBUG "failed to syncronize clipboard: error %i", syncErr));
	}
}

COSXClipboard::~COSXClipboard()
{
	clearConverters();
}

	bool
COSXClipboard::empty()
{
	LOG((CLOG_DEBUG "emptying clipboard"));
	if (m_pboard == NULL)
		return false;

	OSStatus err = PasteboardClear(m_pboard);
	if (err != noErr) {
		LOG((CLOG_DEBUG "failed to clear clipboard: error %i", err));
		return false;
	}

	return true;
}

	bool
COSXClipboard::synchronize()
{
	if (m_pboard == NULL)
		return false;

	PasteboardSyncFlags flags = PasteboardSynchronize(m_pboard);
	LOG((CLOG_DEBUG1 "flags: %x", flags));

	if (flags & kPasteboardModified) {
		return true;
	}
	return false;
}    

	void
COSXClipboard::add(EFormat format, const CString & data)
{
	if (m_pboard == NULL)
		return;

	LOG((CLOG_DEBUG "add %d bytes to clipboard format: %d", data.size(), format));

	for (ConverterList::const_iterator index = m_converters.begin();
			index != m_converters.end(); ++index) {

		IOSXClipboardConverter* converter = *index;

		// skip converters for other formats
		if (converter->getFormat() == format) {
			CString osXData = converter->fromIClipboard(data);
			CFStringRef flavorType = converter->getOSXFormat();
			CFDataRef dataRef = CFDataCreate(kCFAllocatorDefault, (UInt8 *)osXData.data(), osXData.size());
			
			// integ tests showed that if you call add(...) twice, then the
			// second call will actually fail to set clipboard data. calling
			// empty() seems to solve this problem.
			empty();
			
			PasteboardPutItemFlavor(
					m_pboard,
					(PasteboardItemID) 0,
					flavorType,
					dataRef, 
					kPasteboardFlavorNoFlags);
			LOG((CLOG_DEBUG "added %d bytes to clipboard format: %d", data.size(), format));
		}
	}
}

bool
COSXClipboard::open(Time time) const 
{
	if (m_pboard == NULL)
		return false;

	LOG((CLOG_DEBUG "opening clipboard"));
	m_time = time;
	return true;
}

void
COSXClipboard::close() const
{
	LOG((CLOG_DEBUG "closing clipboard"));
	/* not needed */
}

IClipboard::Time
COSXClipboard::getTime() const
{
	return m_time;
}

bool
COSXClipboard::has(EFormat format) const
{
	if (m_pboard == NULL)
		return false;

	PasteboardItemID item;
	PasteboardGetItemIdentifier(m_pboard, (CFIndex) 1, &item);

	for (ConverterList::const_iterator index = m_converters.begin();
			index != m_converters.end(); ++index) {
		IOSXClipboardConverter* converter = *index;
		if (converter->getFormat() == format) {
			PasteboardFlavorFlags flags;
			CFStringRef type = converter->getOSXFormat();

			OSStatus res;

			if ((res = PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags)) == noErr) {
				return true;
			}
		}
	}

	return false;
}

CString
COSXClipboard::get(EFormat format) const
{
	CFStringRef type;
	PasteboardItemID item;
	CString result;

	if (m_pboard == NULL)
		return result;

	PasteboardGetItemIdentifier(m_pboard, (CFIndex) 1, &item);


	// find the converter for the first clipboard format we can handle
	IOSXClipboardConverter* converter = NULL;
	for (ConverterList::const_iterator index = m_converters.begin();
			index != m_converters.end(); ++index) {
		converter = *index;

		PasteboardFlavorFlags flags;
		type = converter->getOSXFormat();

		if (converter->getFormat() == format &&
				PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags) == noErr) {
			break;
		}
		converter = NULL;
	}

	// if no converter then we don't recognize any formats
	if (converter == NULL) {
		LOG((CLOG_DEBUG "Unable to find converter for data"));
		return result;
	}

	// get the clipboard data.
	CFDataRef buffer = NULL;
	try {
		OSStatus err = PasteboardCopyItemFlavorData(m_pboard, item, type, &buffer);

		if (err != noErr) {
			throw err;
		}

		result = CString((char *) CFDataGetBytePtr(buffer), CFDataGetLength(buffer));
	}
	catch (OSStatus err) {
		LOG((CLOG_DEBUG "exception thrown in COSXClipboard::get MacError (%d)", err));
	}
	catch (...) {
		LOG((CLOG_DEBUG "unknown exception in COSXClipboard::get"));
		RETHROW_XTHREAD
	}

	if (buffer != NULL)
		CFRelease(buffer);

	return converter->toIClipboard(result);
}

	void
COSXClipboard::clearConverters()
{
	if (m_pboard == NULL)
		return;

	for (ConverterList::iterator index = m_converters.begin();
			index != m_converters.end(); ++index) {
		delete *index;
	}
	m_converters.clear();
}

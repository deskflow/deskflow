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
}

COSXClipboard::~COSXClipboard()
{
	clearConverters();
}

bool
COSXClipboard::empty()
{
	LOG((CLOG_DEBUG "empty clipboard"));
	assert(m_pboard != NULL);

	OSStatus err = PasteboardClear(m_pboard);
	if (err != noErr) {
		LOG((CLOG_DEBUG "failed to grab clipboard"));
		return false;
	}

    
	// we own the clipboard
	UInt8 claimString[] = {'s', 'y', 'n', 'e'};
	CFDataRef claimData = CFDataCreate(kCFAllocatorDefault, claimString, 4);

	err = PasteboardPutItemFlavor(
				m_pboard,
                0,
				getOwnershipFlavor(),
				claimData,
				kPasteboardFlavorNoFlags);
	if (err != noErr) {
		LOG((CLOG_DEBUG "failed to grab clipboard (%d)", err));
		return false;
	}

	return true;
}

void
COSXClipboard::add(EFormat format, const CString & data)
{
	LOG((CLOG_DEBUG "add %d bytes to clipboard format: %d", data.size(), format));

	for (ConverterList::const_iterator index = m_converters.begin();
								index != m_converters.end(); ++index) {

		IOSXClipboardConverter* converter = *index;

		// skip converters for other formats
		if (converter->getFormat() == format) {
			CString osXData = converter->fromIClipboard(data);
			CFStringRef flavorType = converter->getOSXFormat();
            CFDataRef data_ref = CFDataCreate(kCFAllocatorDefault, (UInt8 *)osXData.data(), osXData.size());

			PasteboardPutItemFlavor(
				m_pboard,
                (PasteboardItemID) 1,
				flavorType,
				data_ref, 
				kPasteboardFlavorNoFlags);
            LOG((CLOG_DEBUG "added %d bytes to clipboard format: %d", data.size(), format));
		}
	}
}

bool
COSXClipboard::open(Time time) const 
{
	LOG((CLOG_DEBUG "open clipboard"));
	m_time = time;
	OSStatus err = PasteboardCreate(kPasteboardClipboard, &m_pboard);
	return (err == noErr);
}

void
COSXClipboard::close() const
{
	LOG((CLOG_DEBUG "close clipboard"));
	m_pboard = NULL;
}

IClipboard::Time
COSXClipboard::getTime() const
{
	return m_time;
}

bool
COSXClipboard::has(EFormat format) const
{
	assert(m_pboard != NULL);

	for (ConverterList::const_iterator index = m_converters.begin();
							index != m_converters.end(); ++index) {
		IOSXClipboardConverter* converter = *index;
		if (converter->getFormat() == format) {
			PasteboardFlavorFlags* flags = NULL;
			CFStringRef type = converter->getOSXFormat();

			if (PasteboardGetItemFlavorFlags(m_pboard, (void *)1, type, flags) == noErr) {
				return true;
			}
		}
	}

	return false;
}

CString
COSXClipboard::get(EFormat format) const
{
	CString result;
  CFStringRef type;

	// find the converter for the first clipboard format we can handle
	IOSXClipboardConverter* converter = NULL;
	for (ConverterList::const_iterator index = m_converters.begin();
							index != m_converters.end(); ++index) {
		converter = *index;

		PasteboardFlavorFlags* flags = NULL;
		type = converter->getOSXFormat();

		if (converter->getFormat() == format &&
	    PasteboardGetItemFlavorFlags(m_pboard, (void *)1, type, flags) == noErr) {
			break;
		}
		converter = NULL;
	}

	// if no converter then we don't recognize any formats
	if (converter == NULL) {
		return result;
	}

	// get the clipboard data.
  CFDataRef buffer = NULL;
	try {
		OSStatus err = PasteboardCopyItemFlavorData(m_pboard, (void *)1,
							type, &buffer);
    
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
	for (ConverterList::iterator index = m_converters.begin();
							index != m_converters.end(); ++index) {
		delete *index;
	}
	m_converters.clear();
}

bool 
COSXClipboard::isOwnedBySynergy()
{
	PasteboardFlavorFlags flags;
  PasteboardRef pboard;
	OSStatus err = PasteboardCreate(kPasteboardClipboard, &pboard);
	if (err == noErr) {
		err = PasteboardGetItemFlavorFlags(pboard, 0, getOwnershipFlavor() , &flags);
	}
	return (err == noErr);
}

CFStringRef 
COSXClipboard::getOwnershipFlavor()
{
	return CFSTR("Syne");
}


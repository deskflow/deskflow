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
	m_scrap(NULL)
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
	assert(m_scrap != NULL);

	OSStatus err = ClearScrap(&m_scrap);
	// XXX -- check err?

	err = PutScrapFlavor(
				m_scrap,
				getOwnershipFlavor(),
				kScrapFlavorMaskNone,
				0,
				0); 

	if (err != noErr) {
		LOG((CLOG_DEBUG "failed to grab clipboard"));
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
			ScrapFlavorType flavorType = converter->getOSXFormat();

			PutScrapFlavor(
				m_scrap,
				flavorType,
				kScrapFlavorMaskNone,
				osXData.size(),
				osXData.data()); 
		}
	}
}

bool
COSXClipboard::open(Time time) const 
{
	LOG((CLOG_DEBUG "open clipboard"));
	m_time = time;
	OSStatus err = GetCurrentScrap(&m_scrap);
	return (err == noErr);
}

void
COSXClipboard::close() const
{
	LOG((CLOG_DEBUG "close clipboard"));
	m_scrap = NULL;
}

IClipboard::Time
COSXClipboard::getTime() const
{
	return m_time;
}

bool
COSXClipboard::has(EFormat format) const
{
	assert(m_scrap != NULL);

	for (ConverterList::const_iterator index = m_converters.begin();
							index != m_converters.end(); ++index) {
		IOSXClipboardConverter* converter = *index;
		if (converter->getFormat() == format) {
			ScrapFlavorFlags flags;
			ScrapFlavorType type = converter->getOSXFormat();

			if (GetScrapFlavorFlags(m_scrap, type, &flags) == noErr) {
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

	// find the converter for the first clipboard format we can handle
	IOSXClipboardConverter* converter = NULL;
	for (ConverterList::const_iterator index = m_converters.begin();
							index != m_converters.end(); ++index) {
		converter = *index;

		ScrapFlavorFlags flags;
		ScrapFlavorType type = converter->getOSXFormat();

		if (converter->getFormat() == format &&
			GetScrapFlavorFlags(m_scrap, type, &flags) == noErr) {
			break;
		}
		converter = NULL;
	}

	// if no converter then we don't recognize any formats
	if (converter == NULL) {
		return result;
	}

	// get the clipboard data.
	char* buffer = NULL;
	try {
		Size flavorSize;
		OSStatus err = GetScrapFlavorSize(m_scrap,
							converter->getOSXFormat(), &flavorSize);
		if (err != noErr) {
			throw err;
		}

		buffer = new char[flavorSize];
		if (buffer == NULL) {
			throw memFullErr;
		}

		err = GetScrapFlavorData(m_scrap,
							converter->getOSXFormat(), &flavorSize, buffer);
		if (err != noErr) {
			throw err;
		}

		result = CString(buffer, flavorSize);
	}
	catch (OSStatus err) {
		LOG((CLOG_DEBUG "exception thrown in COSXClipboard::get MacError (%d)", err));
	}
	catch (...) {
		LOG((CLOG_DEBUG "unknown exception in COSXClipboard::get"));
		RETHROW_XTHREAD
	}
	delete[] buffer;

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
	ScrapFlavorFlags flags;
	ScrapRef scrap;
	OSStatus err = GetCurrentScrap(&scrap);
	if (err == noErr) {
		err = GetScrapFlavorFlags(scrap, getOwnershipFlavor() , &flags);
	}
	return (err == noErr);
}

ScrapFlavorType 
COSXClipboard::getOwnershipFlavor()
{
	return 'Syne';
}

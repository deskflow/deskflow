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

#include "CClipboard.h"

//
// CClipboard
//

CClipboard::CClipboard() :
	m_open(false),
	m_owner(false)
{
	open(0);
	empty();
	close();
}

CClipboard::~CClipboard()
{
	// do nothing
}

bool
CClipboard::empty()
{
	assert(m_open);

	// clear all data
	for (SInt32 index = 0; index < kNumFormats; ++index) {
		m_data[index]  = "";
		m_added[index] = false;
	}

	// save time
	m_timeOwned = m_time;

	// we're the owner now
	m_owner = true;

	return true;
}

void
CClipboard::add(EFormat format, const CString& data)
{
	assert(m_open);
	assert(m_owner);

	m_data[format]  = data;
	m_added[format] = true;
}

bool
CClipboard::open(Time time) const
{
	assert(!m_open);

	m_open = true;
	m_time = time;

	return true;
}

void
CClipboard::close() const
{
	assert(m_open);

	m_open = false;
}

CClipboard::Time
CClipboard::getTime() const
{
	return m_timeOwned;
}

bool
CClipboard::has(EFormat format) const
{
	assert(m_open);
	return m_added[format];
}

CString
CClipboard::get(EFormat format) const
{
	assert(m_open);
	return m_data[format];
}

bool
CClipboard::copy(IClipboard* dst, const IClipboard* src)
{
	assert(dst != NULL);
	assert(src != NULL);

	return copy(dst, src, src->getTime());
}

bool
CClipboard::copy(IClipboard* dst, const IClipboard* src, Time time)
{
	assert(dst != NULL);
	assert(src != NULL);

	bool success = false;
	if (src->open(time)) {
		if (dst->open(time)) {
			if (dst->empty()) {
				for (SInt32 format = 0;
								format != IClipboard::kNumFormats; ++format) {
					IClipboard::EFormat eFormat = (IClipboard::EFormat)format;
					if (src->has(eFormat)) {
						dst->add(eFormat, src->get(eFormat));
					}
				}
				success = true;
			}
			dst->close();
		}
		src->close();
	}

	return success;
}

void
CClipboard::unmarshall(const CString& data, Time time)
{
	const char* index = data.data();

	// clear existing data
	open(time);
	empty();

	// read the number of formats
	const UInt32 numFormats = readUInt32(index);
	index += 4;

	// read each format
	for (UInt32 i = 0; i < numFormats; ++i) {
		// get the format id
		UInt32 format = readUInt32(index);
		index += 4;

		// get the size of the format data
		UInt32 size = readUInt32(index);
		index += 4;

		// save the data if it's a known format.  if either the client
		// or server supports more clipboard formats than the other
		// then one of them will get a format >= kNumFormats here.
		if (format < static_cast<UInt32>(IClipboard::kNumFormats)) {
			m_added[format] = true;
			m_data[format]  = CString(index, size);
		}
		index += size;
	}

	// done
	close();
}

CString
CClipboard::marshall() const
{
	CString data;

	// compute size of marshalled data
	UInt32 size = 4;
	UInt32 numFormats = 0;
	UInt32 format;
	for (format = 0; format != IClipboard::kNumFormats; ++format) {
		if (m_added[format]) {
			++numFormats;
			size += 4 + 4 + m_data[format].size();
		}
	}

	// allocate space
	data.reserve(size);

	// marshall the data
	writeUInt32(&data, numFormats);
	for (format = 0; format != IClipboard::kNumFormats; ++format) {
		if (m_added[format]) {
			writeUInt32(&data, format);
			writeUInt32(&data, m_data[format].size());
			data += m_data[format];
		}
	}

	return data;
}

UInt32
CClipboard::readUInt32(const char* buf) const
{
	const unsigned char* ubuf = reinterpret_cast<const unsigned char*>(buf);
	return	(static_cast<UInt32>(ubuf[0]) << 24) |
			(static_cast<UInt32>(ubuf[1]) << 16) |
			(static_cast<UInt32>(ubuf[2]) <<  8) |
			 static_cast<UInt32>(ubuf[3]);
}

void
CClipboard::writeUInt32(CString* buf, UInt32 v) const
{
	*buf += static_cast<UInt8>((v >> 24) & 0xff);
	*buf += static_cast<UInt8>((v >> 16) & 0xff);
	*buf += static_cast<UInt8>((v >>  8) & 0xff);
	*buf += static_cast<UInt8>( v        & 0xff);
}

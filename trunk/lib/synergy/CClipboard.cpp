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

void
CClipboard::unmarshall(const CString& data, Time time)
{
	IClipboard::unmarshall(this, data, time);
}

CString
CClipboard::marshall() const
{
	return IClipboard::marshall(this);
}

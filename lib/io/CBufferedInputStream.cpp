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

#include "CBufferedInputStream.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "CStopwatch.h"
#include "IJob.h"
#include "XIO.h"
#include <cstring>

//
// CBufferedInputStream
//

CBufferedInputStream::CBufferedInputStream(
				CMutex* mutex, IJob* adoptedCloseCB) :
	m_mutex(mutex),
	m_empty(mutex, true),
	m_closeCB(adoptedCloseCB),
	m_closed(false),
	m_hungup(false)
{
	assert(m_mutex != NULL);
}

CBufferedInputStream::~CBufferedInputStream()
{
	delete m_closeCB;
}

void
CBufferedInputStream::write(const void* buffer, UInt32 n)
{
	if (!m_hungup && n > 0) {
		m_buffer.write(buffer, n);
		m_empty = (m_buffer.getSize() == 0);
		m_empty.broadcast();
	}
}

void
CBufferedInputStream::hangup()
{
	m_hungup = true;
	m_empty.broadcast();
}

UInt32
CBufferedInputStream::readNoLock(void* buffer, UInt32 n, double timeout)
{
	if (m_closed) {
		throw XIOClosed();
	}

	// wait for data, hangup, or timeout
	CStopwatch timer(true);
	while (!m_hungup && m_empty == true) {
		if (!m_empty.wait(timer, timeout)) {
			// timed out
			return (UInt32)-1;
		}
	}

	// read data
	const UInt32 count = m_buffer.getSize();
	if (n > count) {
		n = count;
	}
	if (n > 0) {
		if (buffer != NULL) {
			memcpy(buffer, m_buffer.peek(n), n);
		}
		m_buffer.pop(n);
	}

	// update empty state
	if (m_buffer.getSize() == 0) {
		m_empty = true;
		m_empty.broadcast();
	}
	return n;
}

UInt32
CBufferedInputStream::getSizeNoLock() const
{
	return m_buffer.getSize();
}

void
CBufferedInputStream::close()
{
	CLock lock(m_mutex);
	if (m_closed) {
		throw XIOClosed();
	}

	m_closed = true;
	m_hungup = true;
	m_buffer.pop(m_buffer.getSize());
	m_empty.broadcast();
	if (m_closeCB != NULL) {
		m_closeCB->run();
	}
}

UInt32
CBufferedInputStream::read(void* buffer, UInt32 n, double timeout)
{
	CLock lock(m_mutex);
	return readNoLock(buffer, n, timeout);
}

UInt32
CBufferedInputStream::getSize() const
{
	CLock lock(m_mutex);
	return getSizeNoLock();
}

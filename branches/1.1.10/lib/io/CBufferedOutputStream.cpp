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

#include "CBufferedOutputStream.h"
#include "XIO.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "IJob.h"

//
// CBufferedOutputStream
//

CBufferedOutputStream::CBufferedOutputStream(
				CMutex* mutex, IJob* adoptedCloseCB) :
	m_mutex(mutex),
	m_closeCB(adoptedCloseCB),
	m_empty(mutex, true),
	m_closed(false)
{
	assert(m_mutex != NULL);
}

CBufferedOutputStream::~CBufferedOutputStream()
{
	delete m_closeCB;
}

const void*
CBufferedOutputStream::peek(UInt32 n)
{
	return m_buffer.peek(n);
}

void
CBufferedOutputStream::pop(UInt32 n)
{
	m_buffer.pop(n);
	if (m_buffer.getSize() == 0) {
		m_empty.broadcast();
	}
}

UInt32
CBufferedOutputStream::getSize() const
{
	return m_buffer.getSize();
}

void
CBufferedOutputStream::close()
{
	CLock lock(m_mutex);
	if (m_closed) {
		throw XIOClosed();
	}

	m_closed = true;
	m_buffer.pop(m_buffer.getSize());
	if (m_closeCB != NULL) {
		m_closeCB->run();
	}
}

UInt32
CBufferedOutputStream::write(const void* buffer, UInt32 n)
{
	CLock lock(m_mutex);
	if (m_closed) {
		throw XIOClosed();
	}

	m_buffer.write(buffer, n);
	return n;
}

void
CBufferedOutputStream::flush()
{
	// wait until all data is written
	CLock lock(m_mutex);
	while (m_buffer.getSize() > 0) {
		m_empty.wait();
	}
}

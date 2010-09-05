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

#include "CInputPacketStream.h"
#include "CLock.h"
#include "CStopwatch.h"

//
// CInputPacketStream
//

CInputPacketStream::CInputPacketStream(IInputStream* stream, bool adopt) :
	CInputStreamFilter(stream, adopt),
	m_mutex(),
	m_size(0),
	m_buffer(&m_mutex, NULL)
{
	// do nothing
}

CInputPacketStream::~CInputPacketStream()
{
	// do nothing
}

void
CInputPacketStream::close()
{
	getStream()->close();
}

UInt32
CInputPacketStream::read(void* buffer, UInt32 n, double timeout)
{
	CLock lock(&m_mutex);

	// wait for entire message to be read.  return if stream
	// hungup or timeout.
	switch (waitForFullMessage(timeout)) {
	case kData:
		break;

	case kHungup:
		return 0;

	case kTimedout:
		return (UInt32)-1;
	}

	// limit number of bytes to read to the number of bytes left in the
	// current message.
	if (n > m_size) {
		n = m_size;
	}

	// now read from our buffer
	n = m_buffer.readNoLock(buffer, n, -1.0);
	assert(n <= m_size);
	m_size -= n;

	return n;
}

UInt32
CInputPacketStream::getSize() const
{
	CLock lock(&m_mutex);
	return getSizeNoLock();
}

UInt32
CInputPacketStream::getSizeNoLock() const
{
	CStopwatch timer(true);
	while (!hasFullMessage() && getStream()->getSize() > 0) {
		// read more data
		if (getMoreMessage(-1.0) != kData) {
			// stream hungup
			return 0;
		}
	}

	return m_size;
}

CInputPacketStream::EResult
CInputPacketStream::waitForFullMessage(double timeout) const
{
	CStopwatch timer(true);
	while (!hasFullMessage()) {
		// compute remaining timeout
		double t = timeout - timer.getTime();
		if (timeout >= 0.0 && t <= 0.0) {
			// timeout
			return kTimedout;
		}

		// read more data
		switch (getMoreMessage(t)) {
		case kData:
			break;

		case kHungup:
			// stream hungup
			return kHungup;

		case kTimedout:
			// stream timed out
			return kTimedout;
		}
	}

	return kData;
}

CInputPacketStream::EResult
CInputPacketStream::getMoreMessage(double timeout) const
{
	// read more data
	char buffer[4096];
	UInt32 n = getStream()->read(buffer, sizeof(buffer), timeout);

	// return if stream timed out
	if (n == (UInt32)-1) {
		return kTimedout;
	}

	// return if stream hungup
	if (n == 0) {
		m_buffer.hangup();
		return kHungup;
	}

	// append to our buffer
	m_buffer.write(buffer, n);

	return kData;
}

bool
CInputPacketStream::hasFullMessage() const
{
	// get payload length if we don't have it yet
	if (m_size == 0) {
		// check if length field has been read yet
		if (m_buffer.getSizeNoLock() < 4) {
			// not enough data for payload length
			return false;
		}

		// save payload length
		UInt8 buffer[4];
		UInt32 n = m_buffer.readNoLock(buffer, sizeof(buffer), -1.0);
		assert(n == 4);
		m_size = ((UInt32)buffer[0] << 24) |
				 ((UInt32)buffer[1] << 16) |
				 ((UInt32)buffer[2] << 8) |
				  (UInt32)buffer[3];

		// if payload length is zero then discard null message
		if (m_size == 0) {
			return false;
		}
	}
	assert(m_size > 0);

	// we have the full message when we have at least m_size bytes in
	// the buffer
	return (m_buffer.getSizeNoLock() >= m_size);
}

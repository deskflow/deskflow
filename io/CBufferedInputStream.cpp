#include "CBufferedInputStream.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "IJob.h"
#include "XIO.h"
#include <cstring>

//
// CBufferedInputStream
//

CBufferedInputStream::CBufferedInputStream(
	CMutex* mutex,
	IJob* closeCB) :
	m_mutex(mutex),
	m_empty(mutex, true),
	m_closeCB(closeCB),
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
CBufferedInputStream::write(
	const void* data,
	UInt32 n)
{
	if (!m_hungup && n > 0) {
		m_buffer.write(data, n);
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
CBufferedInputStream::readNoLock(
	void* dst,
	UInt32 n)
{
	if (m_closed) {
		throw XIOClosed();
	}

	// wait for data (or hangup)
	while (!m_hungup && m_empty == true) {
		m_empty.wait();
	}

	// read data
	const UInt32 count = m_buffer.getSize();
	if (n > count) {
		n = count;
	}
	if (n > 0) {
		if (dst != NULL) {
			memcpy(dst, m_buffer.peek(n), n);
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
	if (m_closeCB) {
		m_closeCB->run();
	}
}

UInt32
CBufferedInputStream::read(
	void* dst,
	UInt32 n)
{
	CLock lock(m_mutex);
	return readNoLock(dst, n);
}

UInt32
CBufferedInputStream::getSize() const
{
	CLock lock(m_mutex);
	return getSizeNoLock();
}

#include "CSocketInputStream.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "IJob.h"
#include "XIO.h"
#include <string.h>
#include <assert.h>

//
// CSocketInputStream
//

CSocketInputStream::CSocketInputStream(CMutex* mutex, IJob* closeCB) :
								m_mutex(mutex),
								m_empty(mutex, true),
								m_closeCB(closeCB),
								m_closed(false),
								m_hungup(false)
{
	assert(m_mutex != NULL);
}

CSocketInputStream::~CSocketInputStream()
{
	delete m_closeCB;
}

void					CSocketInputStream::write(
								const void* data, UInt32 n) throw()
{
	if (!m_hungup && n > 0) {
		m_buffer.write(data, n);
		m_empty = (m_buffer.getSize() == 0);
		m_empty.broadcast();
	}
}

void					CSocketInputStream::hangup() throw()
{
	m_hungup = true;
	m_empty.broadcast();
}

void					CSocketInputStream::close() throw(XIO)
{
	CLock lock(m_mutex);
	if (m_closed) {
		throw XIOClosed();
	}

	m_closed = true;
	if (m_closeCB) {
		m_closeCB->run();
	}
}

UInt32					CSocketInputStream::read(
								void* dst, UInt32 n) throw(XIO)
{
	CLock lock(m_mutex);
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
		::memcpy(dst, m_buffer.peek(n), n);
		m_buffer.pop(n);
	}

	// update empty state
	if (m_buffer.getSize() == 0) {
		m_empty = true;
		m_empty.broadcast();
	}
	return n;
}

UInt32					CSocketInputStream::getSize() const throw()
{
	CLock lock(m_mutex);
	return m_buffer.getSize();
}

#include "CBufferedOutputStream.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "IJob.h"
#include "XIO.h"
#include <assert.h>

//
// CBufferedOutputStream
//

CBufferedOutputStream::CBufferedOutputStream(CMutex* mutex, IJob* closeCB) :
								m_mutex(mutex),
								m_closeCB(closeCB),
								m_closed(false)
{
	assert(m_mutex != NULL);
}

CBufferedOutputStream::~CBufferedOutputStream()
{
	delete m_closeCB;
}

const void*				CBufferedOutputStream::peek(UInt32 n)
{
	return m_buffer.peek(n);
}

void					CBufferedOutputStream::pop(UInt32 n)
{
	m_buffer.pop(n);
}

UInt32					CBufferedOutputStream::getSize() const
{
	return m_buffer.getSize();
}

void					CBufferedOutputStream::close()
{
	CLock lock(m_mutex);
	if (m_closed) {
		throw XIOClosed();
	}

	m_closed = true;
	m_buffer.pop(m_buffer.getSize());
	if (m_closeCB) {
		m_closeCB->run();
	}
}

UInt32					CBufferedOutputStream::write(
								const void* data, UInt32 n)
{
	CLock lock(m_mutex);
	if (m_closed) {
		throw XIOClosed();
	}

	m_buffer.write(data, n);
	return n;
}

void					CBufferedOutputStream::flush()
{
	// wait until all data is written
	while (getSizeWithLock() > 0) {
		CThread::sleep(0.05);
	}
}

UInt32					CBufferedOutputStream::getSizeWithLock() const
{
	CLock lock(m_mutex);
	return m_buffer.getSize();
}


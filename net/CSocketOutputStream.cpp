#include "CSocketOutputStream.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "IJob.h"
#include "XIO.h"
#include <assert.h>

//
// CSocketOutputStream
//

CSocketOutputStream::CSocketOutputStream(CMutex* mutex, IJob* closeCB) :
								m_mutex(mutex),
								m_closeCB(closeCB),
								m_closed(false)
{
	assert(m_mutex != NULL);
}

CSocketOutputStream::~CSocketOutputStream()
{
	delete m_closeCB;
}

const void*				CSocketOutputStream::peek(UInt32 n)
{
	return m_buffer.peek(n);
}

void					CSocketOutputStream::pop(UInt32 n)
{
	m_buffer.pop(n);
}

UInt32					CSocketOutputStream::getSize() const
{
	return m_buffer.getSize();
}

void					CSocketOutputStream::close()
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

UInt32					CSocketOutputStream::write(
								const void* data, UInt32 n)
{
	CLock lock(m_mutex);
	if (m_closed) {
		throw XIOClosed();
	}

	m_buffer.write(data, n);
	return n;
}

void					CSocketOutputStream::flush()
{
	// wait until all data is written
	while (getSizeWithLock() > 0) {
		CThread::sleep(0.05);
	}
}

UInt32					CSocketOutputStream::getSizeWithLock() const
{
	CLock lock(m_mutex);
	return m_buffer.getSize();
}

#include "CMessageSocket.h"
#include "TMethodJob.h"
#include "CTrace.h"
#include <assert.h>
#include <string.h>

//
// CMessageSocket
//

CMessageSocket::CMessageSocket(ISocket* socket) :
								m_socket(socket),
								m_buffer(NULL),
								m_size(0),
								m_capacity(0),
								m_msgSize(0)
{
	m_socket->setReadJob(new TMethodJob<CMessageSocket>(this,
								&CMessageSocket::readJobCB));
}

CMessageSocket::~CMessageSocket()
{
	delete m_socket;
	delete[] m_buffer;
}

void					CMessageSocket::setWriteJob(IJob* adoptedJob)
{
	CSocket::setWriteJob(adoptedJob);
	if (adoptedJob != NULL)
		m_socket->setWriteJob(new TMethodJob<CMessageSocket>(this,
								&CMessageSocket::writeJobCB));
	else
		m_socket->setWriteJob(NULL);
}

void					CMessageSocket::connect(const CString&, UInt16)
{
	assert(0 && "connect() illegal on CMessageSocket");
}

void					CMessageSocket::listen(const CString&, UInt16)
{
	assert(0 && "listen() illegal on CMessageSocket");
}

ISocket*				CMessageSocket::accept()
{
	assert(0 && "accept() illegal on CMessageSocket");
	return NULL;
}

SInt32					CMessageSocket::read(void* buffer, SInt32 n)
{
	// if we don't have an entire message yet then read more data
	if (m_size == 0 || m_size < m_msgSize) {
		doRead();
	}

	// if we don't have a whole message yet then return 0
	if (m_size < m_msgSize)
		return 0;

	// how many bytes should we return?
	if (m_msgSize - 2 < n)
		n = m_msgSize - 2;

	// copy data
	// FIXME -- should have method for retrieving size of next message
	::memcpy(buffer, m_buffer + 2, n);

	// discard returned message
	::memmove(m_buffer, m_buffer + m_msgSize, m_size - m_msgSize);
	m_size   -= m_msgSize;
	m_msgSize = 0;

	// get next message size
	if (m_size >= 2) {
		m_msgSize = static_cast<SInt32>(
								(static_cast<UInt32>(m_buffer[1]) << 8) +
								(static_cast<UInt32>(m_buffer[2])     ));
		TRACE(("  next message size: %d", m_msgSize));
	}

	return n;
}

void					CMessageSocket::write(const void* buffer, SInt32 n)
{
	// FIXME -- no fixed size buffers
	char tmp[512];
	assert(n < (SInt32)sizeof(tmp) - 2);
	::memcpy(tmp + 2, buffer, n);
	n += 2;
	tmp[0] = static_cast<char>((n >> 8) & 0xff);
	tmp[1] = static_cast<char>(n & 0xff);
	m_socket->write(tmp, n);
}

SInt32					CMessageSocket::doRead()
{
	// if read buffer is full then grow it
	if (m_size == m_capacity) {
		// compute new capacity and allocate space
		SInt32 newCapacity = (m_capacity < 256) ? 256 : 2 * m_capacity;
		UInt8* newBuffer = new UInt8[newCapacity];

		// cut over
		::memcpy(newBuffer, m_buffer, m_size);
		delete[] m_buffer;
		m_buffer   = newBuffer;
		m_capacity = newCapacity;
	}

	// read as much data as possible
	const SInt32 numRead = m_socket->read(m_buffer + m_size,
												m_capacity - m_size);
	TRACE(("socket %p read %d bytes", this, numRead));

	// hangup is a special case.  if buffer isn't empty then we'll
	// discard the partial message.
	if (numRead == -1)
		return numRead;

	// get next message size
	if (m_size < 2 && m_size + numRead >= 2) {
		m_msgSize = static_cast<SInt32>(
								(static_cast<UInt32>(m_buffer[0]) << 8) +
								(static_cast<UInt32>(m_buffer[1])     ));
		TRACE(("  next message size: %d", m_msgSize));
	}

	m_size += numRead;
	return numRead;
}

void					CMessageSocket::readJobCB()
{
	if (doRead() == -1) {
		// remote side hungup.  don't check for readability anymore.
		m_socket->setReadJob(NULL);
	}
	else if (m_size > 0 && m_size >= m_msgSize) {
		TRACE(("  message ready"));
		runReadJob();
	}
}

void					CMessageSocket::writeJobCB()
{
	runWriteJob();
}

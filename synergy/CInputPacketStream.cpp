#include "CInputPacketStream.h"
#include "CLock.h"

//
// CInputPacketStream
//

CInputPacketStream::CInputPacketStream(
	IInputStream* stream,
	bool adopt) :
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
CInputPacketStream::read(
	void* buffer,
	UInt32 n)
{
	CLock lock(&m_mutex);

	// wait for entire message to be read.  return immediately if
	// stream hungup.
	if (getSizeNoLock() == 0) {
		return 0;
	}

	// limit number of bytes to read to the number of bytes left in the
	// current message.
	if (n > m_size) {
		n = m_size;
	}

	// now read from our buffer
	n = m_buffer.readNoLock(buffer, n);
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
	while (!hasFullMessage()) {
		// read more data
		char buffer[4096];
		UInt32 n = getStream()->read(buffer, sizeof(buffer));

		// return if stream hungup
		if (n == 0) {
			m_buffer.hangup();
			return 0;
		}

		// append to our buffer
		m_buffer.write(buffer, n);
	}

	return m_size;
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
		UInt32 n = m_buffer.readNoLock(buffer, sizeof(buffer));
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

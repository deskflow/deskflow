#include "CTCPSocket.h"
#include "CBufferedInputStream.h"
#include "CBufferedOutputStream.h"
#include "CNetworkAddress.h"
#include "XIO.h"
#include "XSocket.h"
#include "CCondVar.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "CStopwatch.h"
#include "TMethodJob.h"

//
// CTCPSocket
//

CTCPSocket::CTCPSocket()
{
	m_fd = CNetwork::socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd == CNetwork::Null) {
		throw XSocketCreate();
	}
	init();
}

CTCPSocket::CTCPSocket(CNetwork::Socket fd) :
	m_fd(fd)
{
	assert(m_fd != CNetwork::Null);

	init();

	// socket starts in connected state
	m_connected = kReadWrite;

	// start handling socket
	m_thread = new CThread(new TMethodJob<CTCPSocket>(
								this, &CTCPSocket::ioThread));
}

CTCPSocket::~CTCPSocket()
{
	try {
		close();
	}
	catch (...) {
		// ignore failures
	}

	// clean up
	delete m_input;
	delete m_output;
	delete m_mutex;
}

void
CTCPSocket::bind(const CNetworkAddress& addr)
{
	if (CNetwork::bind(m_fd, addr.getAddress(),
								addr.getAddressLength()) == CNetwork::Error) {
		if (errno == CNetwork::kEADDRINUSE) {
			throw XSocketAddressInUse();
		}
		throw XSocketBind();
	}
}

void
CTCPSocket::close()
{
	// see if buffers should be flushed
	bool doFlush = false;
	{
		CLock lock(m_mutex);
		doFlush = (m_thread != NULL && (m_connected & kWrite) != 0);
	}

	// flush buffers
	if (doFlush) {
		m_output->flush();
	}

	// cause ioThread to exit
	{
		CLock lock(m_mutex);
		if (m_fd != CNetwork::Null) {
			CNetwork::shutdown(m_fd, 2);
			m_connected = kClosed;
		}
	}

	// wait for thread
	if (m_thread != NULL) {
		m_thread->wait();
		delete m_thread;
		m_thread = NULL;
	}

	// close socket
	if (m_fd != CNetwork::Null) {
		if (CNetwork::close(m_fd) == CNetwork::Error) {
			throw XIOClose();
		}
		m_fd = CNetwork::Null;
	}
}

void
CTCPSocket::connect(const CNetworkAddress& addr)
{
	CThread::testCancel();
	if (CNetwork::connect(m_fd, addr.getAddress(),
								addr.getAddressLength()) == CNetwork::Error) {
		CThread::testCancel();
		throw XSocketConnect();
	}

	// start servicing the socket
	m_connected = kReadWrite;
	m_thread    = new CThread(new TMethodJob<CTCPSocket>(
								this, &CTCPSocket::ioThread));
}

IInputStream*
CTCPSocket::getInputStream()
{
	return m_input;
}

IOutputStream*
CTCPSocket::getOutputStream()
{
	return m_output;
}

void
CTCPSocket::init()
{
	m_mutex     = new CMutex;
	m_thread    = NULL;
	m_connected = kClosed;
	m_input     = new CBufferedInputStream(m_mutex,
								new TMethodJob<CTCPSocket>(
									this, &CTCPSocket::closeInput));
	m_output    = new CBufferedOutputStream(m_mutex,
								new TMethodJob<CTCPSocket>(
									this, &CTCPSocket::closeOutput));

	// turn off Nagle algorithm.  we send lots of very short messages
	// that should be sent without (much) delay.  for example, the
	// mouse motion messages are much less useful if they're delayed.
	CNetwork::TCPNoDelayType flag = 1;
	CNetwork::setsockopt(m_fd, SOL_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

void
CTCPSocket::ioThread(void*)
{
	try {
		ioService();
		ioCleanup();
	}
	catch (...) {
		ioCleanup();
		throw;
	}
}

void
CTCPSocket::ioCleanup()
{
	try {
		m_input->close();
	}
	catch (...) {
		// ignore
	}
	try {
		m_output->close();
	}
	catch (...) {
		// ignore
	}
}

void
CTCPSocket::ioService()
{
	assert(m_fd != CNetwork::Null);

	// now service the connection
	CNetwork::PollEntry pfds[1];
	pfds[0].fd = m_fd;
	for (;;) {
		{
			// choose events to poll for
			CLock lock(m_mutex);
			pfds[0].events = 0;
			if (m_connected == 0) {
				return;
			}
			if ((m_connected & kRead) != 0) {
				// still open for reading
				pfds[0].events |= CNetwork::kPOLLIN;
			}
			if ((m_connected & kWrite) != 0 && m_output->getSize() > 0) {
				// data queued for writing
				pfds[0].events |= CNetwork::kPOLLOUT;
			}
		}

		// check for status
		const int status = CNetwork::poll(pfds, 1, 10);

		// transfer data and handle errors
		if (status == 1) {
			if ((pfds[0].revents & (CNetwork::kPOLLERR |
									CNetwork::kPOLLNVAL)) != 0) {
				// stream is no good anymore so bail
				m_input->hangup();
				return;
			}

			// read some data
			if (pfds[0].revents & CNetwork::kPOLLIN) {
				UInt8 buffer[4096];
				ssize_t n = CNetwork::read(m_fd, buffer, sizeof(buffer));
				if (n > 0) {
					CLock lock(m_mutex);
					m_input->write(buffer, n);
				}
				else if (n == 0) {
					// stream hungup
					m_input->hangup();
					m_connected &= ~kRead;
				}
			}

			// write some data
			if (pfds[0].revents & CNetwork::kPOLLOUT) {
				CLock lock(m_mutex);

				// get amount of data to write
				UInt32 n = m_output->getSize();

				// write data
				const void* buffer = m_output->peek(n);
				n = (UInt32)CNetwork::write(m_fd, buffer, n);

				// discard written data
				if (n > 0) {
					m_output->pop(n);
				}
				else if (n == (UInt32)-1 && CNetwork::getsockerror() == EPIPE) {
					return;
				}
			}
		}
	}
}

void
CTCPSocket::closeInput(void*)
{
	// note -- m_mutex should already be locked
	CNetwork::shutdown(m_fd, 0);
	m_connected &= ~kRead;
}

void
CTCPSocket::closeOutput(void*)
{
	// note -- m_mutex should already be locked
	CNetwork::shutdown(m_fd, 1);
	m_connected &= ~kWrite;
}

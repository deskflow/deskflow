#include "CTCPSocket.h"
#include "CBufferedInputStream.h"
#include "CBufferedOutputStream.h"
#include "CNetworkAddress.h"
#include "CLock.h"
#include "CMutex.h"
#include "CCondVar.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CStopwatch.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <assert.h>

//
// CTCPSocket
//

CTCPSocket::CTCPSocket() throw(XSocket)
{
	m_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd == -1) {
		throw XSocketCreate();
	}
	init();
}

CTCPSocket::CTCPSocket(int fd) throw() :
								m_fd(fd)
{
	assert(m_fd != -1);

	init();

	// socket starts in connected state
	m_connected = kReadWrite;

	// start handling socket
	m_thread = new CThread(new TMethodJob<CTCPSocket>(
								this, &CTCPSocket::service));
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
	delete m_mutex;
	delete m_input;
	delete m_output;
}

void					CTCPSocket::bind(const CNetworkAddress& addr)
								throw(XSocket)
{
	if (::bind(m_fd, addr.getAddress(), addr.getAddressLength()) == -1) {
		if (errno == EADDRINUSE) {
			throw XSocketAddressInUse();
		}
		throw XSocketBind();
	}
}

void					CTCPSocket::connect(const CNetworkAddress& addr)
								throw(XSocket)
{
	CThread::testCancel();
	if (::connect(m_fd, addr.getAddress(), addr.getAddressLength()) == -1) {
		CThread::testCancel();
		throw XSocketConnect();
	}

	// start servicing the socket
	m_connected = kReadWrite;
	m_thread    = new CThread(new TMethodJob<CTCPSocket>(
								this, &CTCPSocket::service));
}

void					CTCPSocket::close() throw(XIO)
{
	// shutdown I/O thread before close
	if (m_thread != NULL) {
		// flush if output buffer not empty and output buffer not closed
		bool doFlush;
		{
			CLock lock(m_mutex);
			doFlush = ((m_connected & kWrite) != 0);
		}
		if (doFlush) {
			m_output->flush();
		}

		m_thread->cancel();
		m_thread->wait();
		delete m_thread;
		m_thread = NULL;
	}

	CLock lock(m_mutex);
	if (m_fd != -1) {
		if (::close(m_fd) == -1) {
			throw XIOClose();
		}
		m_fd = -1;
	}
}

IInputStream*			CTCPSocket::getInputStream() throw()
{
	return m_input;
}

IOutputStream*			CTCPSocket::getOutputStream() throw()
{
	return m_output;
}

void					CTCPSocket::init() throw(XIO)
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
}

void					CTCPSocket::service(void*) throw(XThread)
{
	assert(m_fd != -1);

	// now service the connection
	struct pollfd pfds[1];
	pfds[0].fd = m_fd;
	for (;;) {
		{
			// choose events to poll for
			CLock lock(m_mutex);
			pfds[0].events = 0;
			if ((m_connected & kRead) != 0) {
				// still open for reading
				pfds[0].events |= POLLIN;
			}
			if ((m_connected & kWrite) != 0 && m_output->getSize() > 0) {
				// data queued for writing
				pfds[0].events |= POLLOUT;
			}
		}

		// check for status
		CThread::testCancel();
		const int status = poll(pfds, 1, 50);
		CThread::testCancel();

		// transfer data and handle errors
		if (status == 1) {
			if ((pfds[0].revents & (POLLERR | POLLNVAL)) != 0) {
				// stream is no good anymore so bail
				m_input->hangup();
				return;
			}

			// read some data
			if (pfds[0].revents & POLLIN) {
				UInt8 buffer[4096];
				ssize_t n = read(m_fd, buffer, sizeof(buffer));
				if (n > 0) {
					CLock lock(m_mutex);
					m_input->write(buffer, n);
				}
				else if (n == 0) {
					// stream hungup
					m_input->hangup();
					return;
				}
			}

			// write some data
			if (pfds[0].revents & POLLOUT) {
				CLock lock(m_mutex);

				// get amount of data to write
				UInt32 n = m_output->getSize();
				if (n > 4096) {
					// limit write size
					n = 4096;
				}

				// write data
				const void* buffer = m_output->peek(n);
				n = write(m_fd, buffer, n);

				// discard written data
				if (n > 0) {
					m_output->pop(n);
				}
			}
		}
	}
}

void					CTCPSocket::closeInput(void*) throw()
{
	// note -- m_mutex should already be locked
	shutdown(m_fd, 0);
	m_connected &= ~kRead;
}

void					CTCPSocket::closeOutput(void*) throw()
{
	// note -- m_mutex should already be locked
	shutdown(m_fd, 1);
	m_connected &= ~kWrite;
}

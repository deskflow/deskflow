#include "CUnixTCPSocket.h"
#include "CUnixEventQueue.h"
#include "CString.h"
#include "TMethodJob.h"
#include "XSocket.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> // FIXME -- for disabling nagle algorithm
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
extern int h_errno;

CUnixTCPSocket::CUnixTCPSocket() : m_fd(-1),
								m_state(kNone),
								m_addedJobs(false)
{
	// create socket
	m_fd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd == -1)
		throw XSocketCreate(::strerror(errno));

	// make it non-blocking
	int mode = ::fcntl(m_fd, F_GETFL, 0);
	if (mode == -1 || ::fcntl(m_fd, F_SETFL, mode | O_NONBLOCK) == -1) {
		::close(m_fd);
		throw XSocketCreate(::strerror(errno));
	}

	// turn off Nagle algorithm.  we send lots of really short messages.
	struct protoent* p = getprotobyname("tcp");
	if (p) {
		int on = 1;
		setsockopt(m_fd, p->p_proto, TCP_NODELAY, &on, sizeof(on));
	}
}

CUnixTCPSocket::CUnixTCPSocket(int fd) : m_fd(fd),
								m_state(kConnected),
								m_addedJobs(false)
{
	assert(m_fd != -1);
}

CUnixTCPSocket::~CUnixTCPSocket()
{
	assert(m_fd != -1);

	// unhook events
	if (m_addedJobs)
		CEQ->removeFileDesc(m_fd);

	// drain socket
	if (m_state == kConnected)
		::shutdown(m_fd, 0);

	// close socket
	::close(m_fd);
}

void					CUnixTCPSocket::onJobChanged()
{
	// remove old jobs
	if (m_addedJobs) {
		CEQ->removeFileDesc(m_fd);
		m_addedJobs = false;
	}

	// which jobs should we install?
	bool doRead  = false;
	bool doWrite = false;
	switch (m_state) {
	  case kNone:
		return;

	  case kConnecting:
		doWrite = true;
		break;

	  case kConnected:
		doRead  = hasReadJob();
		doWrite = hasWriteJob();
		break;

	  case kListening:
		doRead = true;
		break;
	}

	// make jobs
	IJob* readJob  = doRead  ? new TMethodJob<CUnixTCPSocket>(this,
										&CUnixTCPSocket::readCB) : NULL;
	IJob* writeJob = doWrite ? new TMethodJob<CUnixTCPSocket>(this,
										&CUnixTCPSocket::writeCB) : NULL;

	// install jobs
	CEQ->addFileDesc(m_fd, readJob, writeJob);
	m_addedJobs = true;
}

void					CUnixTCPSocket::readCB()
{
	runReadJob();
}

void					CUnixTCPSocket::writeCB()
{
	if (m_state == kConnecting) {
		// now connected.  start watching for reads.
		m_state = kConnected;
		onJobChanged();
	}
	runWriteJob();
}

void					CUnixTCPSocket::connect(
								const CString& hostname, UInt16 port)
{
	assert(m_fd != -1);
	assert(m_state == kNone);

	// hostname to address
	struct hostent* hent = ::gethostbyname(hostname.c_str());
	if (hent == NULL)
		throw XSocketName(::hstrerror(h_errno));

	// construct address
	struct sockaddr_in addr;
	assert(hent->h_addrtype == AF_INET);
	assert(hent->h_length   == sizeof(addr.sin_addr));
	addr.sin_family = hent->h_addrtype;
	addr.sin_port   = htons(port);
	::memcpy(&addr.sin_addr, hent->h_addr_list[0], hent->h_length);

	// start connecting
	if (::connect(m_fd, reinterpret_cast<struct sockaddr*>(&addr),
											sizeof(addr)) == -1) {
		if (errno != EINPROGRESS)
			throw XSocketConnect(::strerror(errno));
		m_state = kConnecting;
	}
	else {
		m_state = kConnected;
		runWriteJob();
	}
	onJobChanged();
}

void					CUnixTCPSocket::listen(
								const CString& hostname, UInt16 port)
{
	assert(m_fd != -1);
	assert(m_state == kNone);
	assert(port != 0);

	// construct address
	struct sockaddr_in addr;
	if (!hostname.empty()) {
		// hostname to address
		struct hostent* hent = ::gethostbyname(hostname.c_str());
		if (hent == NULL)
			throw XSocketName(::hstrerror(h_errno));

		// fill in address
		assert(hent->h_addrtype == AF_INET);
		assert(hent->h_length   == sizeof(addr.sin_addr));
		::memcpy(&addr.sin_addr, hent->h_addr_list[0], hent->h_length);
	}
	else {
		// all addresses
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);

	// bind to address
	if (::bind(m_fd, reinterpret_cast<struct sockaddr*>(&addr),
											sizeof(addr)) == -1)
		throw XSocketListen(::strerror(errno));

	// start listening
	if (::listen(m_fd, 3) == -1)
		throw XSocketListen(::strerror(errno));
	m_state = kListening;
	onJobChanged();
}

ISocket*				CUnixTCPSocket::accept()
{
	assert(m_fd != -1);
	assert(m_state == kListening);

	for (;;) {
		// wait for connection
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(m_fd, &fdset);
		::select(m_fd + 1, &fdset, NULL, NULL, NULL);

		// accept connection
		struct sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		int fd = ::accept(m_fd, &addr, &addrlen);
		if (fd == -1)
			if (errno == EAGAIN)
				continue;
			else
				throw XSocketAccept(::strerror(errno));

		// return new socket object
		return new CUnixTCPSocket(fd);
	}
}

SInt32					CUnixTCPSocket::read(void* buffer, SInt32 numBytes)
{
	assert(m_fd != -1);
	assert(m_state == kConnected);

	const ssize_t n = ::read(m_fd, buffer, numBytes);
	if (n == -1) {
		// check for no data to read
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		// error
		return -1;
	}

	// check for socket closed
	if (n == 0)
		return -1;

	// return num bytes read
	return n;
}

void					CUnixTCPSocket::write(
								const void* buffer, SInt32 numBytes)
{
	const char* ptr = static_cast<const char*>(buffer);

	while (numBytes > 0) {
		// write more data
		const ssize_t n = ::write(m_fd, ptr, numBytes);

		// check for errors
		if (n == -1) {
			// wait if can't write data then try again
			if (errno == EAGAIN || errno == EINTR) {
				fd_set fdset;
				FD_ZERO(&fdset);
				FD_SET(m_fd, &fdset);
				::select(m_fd + 1, NULL, &fdset, NULL, NULL);
				continue;
			}

			// error
			throw XSocketWrite(::strerror(errno));
		}

		// account for written data
		ptr += n;
		numBytes -= n;
	}
}

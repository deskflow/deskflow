#include "CTCPListenSocket.h"
#include "CTCPSocket.h"
#include "CNetworkAddress.h"
#include "CThread.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//
// CTCPListenSocket
//

CTCPListenSocket::CTCPListenSocket()
{
	m_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd == -1) {
		throw XSocketCreate();
	}
}

CTCPListenSocket::~CTCPListenSocket()
{
	try {
		close();
	}
	catch (...) {
		// ignore
	}
}

void					CTCPListenSocket::bind(
								const CNetworkAddress& addr) throw(XSocket)
{
	if (::bind(m_fd, addr.getAddress(), addr.getAddressLength()) == -1) {
		if (errno == EADDRINUSE) {
			throw XSocketAddressInUse();
		}
		throw XSocketBind();
	}
	if (listen(m_fd, 3) == -1) {
		throw XSocketBind();
	}
}

ISocket*				CTCPListenSocket::accept() throw(XSocket)
{
	for (;;) {
		struct sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		CThread::testCancel();
		int fd = ::accept(m_fd, &addr, &addrlen);
		if (fd == -1) {
			CThread::testCancel();
		}
		else {
			return new CTCPSocket(fd);
		}
	}
}

void					CTCPListenSocket::close() throw(XIO)
{
	if (m_fd == -1) {
		throw XIOClosed();
	}
	if (::close(m_fd) == -1) {
		throw XIOClose();
	}
	m_fd = -1;
}

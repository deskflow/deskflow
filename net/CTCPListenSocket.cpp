#include "CTCPListenSocket.h"
#include "CTCPSocket.h"
#include "CNetworkAddress.h"
#include "XIO.h"
#include "XSocket.h"
#include "CThread.h"

//
// CTCPListenSocket
//

CTCPListenSocket::CTCPListenSocket()
{
	m_fd = CNetwork::socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd == CNetwork::Null) {
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

void
CTCPListenSocket::bind(
	const CNetworkAddress& addr)
{
	if (CNetwork::bind(m_fd, addr.getAddress(),
								addr.getAddressLength()) == CNetwork::Error) {
		if (CNetwork::getsockerror() == CNetwork::kEADDRINUSE) {
			throw XSocketAddressInUse();
		}
		throw XSocketBind();
	}
	if (CNetwork::listen(m_fd, 3) == CNetwork::Error) {
		throw XSocketBind();
	}
}

IDataSocket*
CTCPListenSocket::accept()
{
	CNetwork::PollEntry pfds[1];
	pfds[0].fd     = m_fd;
	pfds[0].events = CNetwork::kPOLLIN;
	for (;;) {
		CThread::testCancel();
		const int status = CNetwork::poll(pfds, 1, 10);
		if (status > 0 && (pfds[0].revents & CNetwork::kPOLLIN) != 0) {
			CNetwork::Address addr;
			CNetwork::AddressLength addrlen = sizeof(addr);
			CNetwork::Socket fd = CNetwork::accept(m_fd, &addr, &addrlen);
			if (fd != CNetwork::Null) {
				return new CTCPSocket(fd);
			}
		}
	}
}

void
CTCPListenSocket::close()
{
	if (m_fd == CNetwork::Null) {
		throw XIOClosed();
	}
	if (CNetwork::close(m_fd) == CNetwork::Error) {
		throw XIOClose();
	}
	m_fd = CNetwork::Null;
}

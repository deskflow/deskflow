/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CTCPListenSocket.h"
#include "CTCPSocket.h"
#include "CNetworkAddress.h"
#include "XIO.h"
#include "XSocket.h"
#include "CThread.h"
#include "CArch.h"
#include "XArch.h"

//
// CTCPListenSocket
//

CTCPListenSocket::CTCPListenSocket()
{
	try {
		m_socket = ARCH->newSocket(IArchNetwork::kINET, IArchNetwork::kSTREAM);
	}
	catch (XArchNetwork& e) {
		throw XSocketCreate(e.what());
	}
}

CTCPListenSocket::~CTCPListenSocket()
{
	try {
		ARCH->closeSocket(m_socket);
	}
	catch (...) {
		// ignore
	}
}

void
CTCPListenSocket::bind(const CNetworkAddress& addr)
{
	try {
		ARCH->bindSocket(m_socket, addr.getAddress());
		ARCH->listenOnSocket(m_socket);
	}
	catch (XArchNetworkAddressInUse& e) {
		throw XSocketAddressInUse(e.what());
	}
	catch (XArchNetwork& e) {
		throw XSocketBind(e.what());
	}
}

IDataSocket*
CTCPListenSocket::accept()
{
	// accept asynchronously so we can check for cancellation
	IArchNetwork::CPollEntry pfds[1];
	pfds[0].m_socket = m_socket;
	pfds[0].m_events = IArchNetwork::kPOLLIN;
	for (;;) {
		ARCH->testCancelThread();
		try {
			const int status = ARCH->pollSocket(pfds, 1, 0.01);
			if (status > 0 &&
				(pfds[0].m_revents & IArchNetwork::kPOLLIN) != 0) {
				return new CTCPSocket(ARCH->acceptSocket(m_socket, NULL));
			}
		}
		catch (XArchNetwork&) {
			// ignore and retry
		}
	}
}

void
CTCPListenSocket::close()
{
	if (m_socket == NULL) {
		throw XIOClosed();
	}
	try {
		ARCH->closeSocket(m_socket);
		m_socket = NULL;
	}
	catch (XArchNetwork& e) {
		throw XSocketIOClose(e.what());
	}
}

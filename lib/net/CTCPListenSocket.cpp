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
#include "CNetworkAddress.h"
#include "CSocketMultiplexer.h"
#include "CTCPSocket.h"
#include "TSocketMultiplexerMethodJob.h"
#include "XSocket.h"
#include "XIO.h"
#include "CEvent.h"
#include "CEventQueue.h"
#include "CLock.h"
#include "CMutex.h"
#include "CArch.h"
#include "XArch.h"

//
// CTCPListenSocket
//

CTCPListenSocket::CTCPListenSocket() :
	m_target(NULL)
{
	m_mutex = new CMutex;
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
		if (m_socket != NULL) {
			CSocketMultiplexer::getInstance()->removeSocket(this);
			ARCH->closeSocket(m_socket);
		}
	}
	catch (...) {
		// ignore
	}
	delete m_mutex;
}

void
CTCPListenSocket::bind(const CNetworkAddress& addr)
{
	try {
		CLock lock(m_mutex);
		ARCH->bindSocket(m_socket, addr.getAddress());
		ARCH->listenOnSocket(m_socket);
		CSocketMultiplexer::getInstance()->addSocket(this,
							new TSocketMultiplexerMethodJob<CTCPListenSocket>(
								this, &CTCPListenSocket::serviceListening,
								m_socket, true, false));
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
	try {
		CSocketMultiplexer::getInstance()->addSocket(this,
							new TSocketMultiplexerMethodJob<CTCPListenSocket>(
								this, &CTCPListenSocket::serviceListening,
								m_socket, true, false));
		return new CTCPSocket(ARCH->acceptSocket(m_socket, NULL));
	}
	catch (XArchNetwork&) {
		return NULL;
	}
}

void
CTCPListenSocket::close()
{
	CLock lock(m_mutex);
	if (m_socket == NULL) {
		throw XIOClosed();
	}
	try {
		CSocketMultiplexer::getInstance()->removeSocket(this);
		ARCH->closeSocket(m_socket);
		m_socket = NULL;
	}
	catch (XArchNetwork& e) {
		throw XSocketIOClose(e.what());
	}
}

void
CTCPListenSocket::setEventTarget(void* target)
{
	CLock lock(m_mutex);
	m_target = target;
}

ISocketMultiplexerJob*
CTCPListenSocket::serviceListening(ISocketMultiplexerJob* job,
							bool read, bool, bool error)
{
	if (error) {
		close();
		return NULL;
	}
	if (read) {
		CEventQueue::getInstance()->addEvent(
							CEvent(getConnectingEvent(), m_target, NULL));
		// stop polling on this socket until the client accepts
		return NULL;
	}
	return job;
}

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

#include "CTCPSocket.h"
#include "CNetworkAddress.h"
#include "CSocketMultiplexer.h"
#include "TSocketMultiplexerMethodJob.h"
#include "CBufferedInputStream.h"
#include "CBufferedOutputStream.h"
#include "XSocket.h"
#include "XIO.h"
#include "CLock.h"
#include "CMutex.h"
#include "CEventQueue.h"
#include "TMethodJob.h"
#include "CArch.h"
#include "XArch.h"

//
// CTCPSocket
//

CTCPSocket::CTCPSocket()
{
	try {
		m_socket = ARCH->newSocket(IArchNetwork::kINET, IArchNetwork::kSTREAM);
	}
	catch (XArchNetwork& e) {
		throw XSocketCreate(e.what());
	}
	init();
}

CTCPSocket::CTCPSocket(CArchSocket socket) :
	m_socket(socket)
{
	assert(m_socket != NULL);

	// socket starts in connected state
	init();
	setState(kReadWrite, true);
}

CTCPSocket::~CTCPSocket()
{
	try {
		if (m_socket != NULL) {
			close();
		}
	}
	catch (...) {
		// ignore
	}

	// clean up
	delete m_input;
	delete m_output;
	delete m_mutex;
}

void
CTCPSocket::bind(const CNetworkAddress& addr)
{
	try {
		ARCH->bindSocket(m_socket, addr.getAddress());
	}
	catch (XArchNetworkAddressInUse& e) {
		throw XSocketAddressInUse(e.what());
	}
	catch (XArchNetwork& e) {
		throw XSocketBind(e.what());
	}
}

void
CTCPSocket::close()
{
	// flush buffers
	m_output->flush();

	// now closed
	setState(kClosed, true);

	// close buffers
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

	// close socket
	CLock lock(m_mutex);
	if (m_socket != NULL) {
		try {
			ARCH->closeSocket(m_socket);
			m_socket = NULL;
		}
		catch (XArchNetwork& e) {
			throw XSocketIOClose(e.what());
		}
	}
}

void
CTCPSocket::setEventTarget(void* target)
{
	CLock lock(m_mutex);
	m_target = target;
}

void
CTCPSocket::connect(const CNetworkAddress& addr)
{
	try {
// FIXME -- don't throw if in progress, just return that info
		ARCH->connectSocket(m_socket, addr.getAddress());
		setState(kReadWrite, true);
	}
	catch (XArchNetworkConnecting&) {
		// connection is in progress
		setState(kConnecting, true);
	}
	catch (XArchNetwork& e) {
		throw XSocketConnect(e.what());
	}
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
	m_mutex  = new CMutex;
	m_input  = new CBufferedInputStream(m_mutex,
								new TMethodJob<CTCPSocket>(
									this, &CTCPSocket::emptyInput),
								new TMethodJob<CTCPSocket>(
									this, &CTCPSocket::closeInput));
	m_output = new CBufferedOutputStream(m_mutex,
								new TMethodJob<CTCPSocket>(
									this, &CTCPSocket::fillOutput),
								new TMethodJob<CTCPSocket>(
									this, &CTCPSocket::closeOutput));
	m_state  = kUnconnected;
	m_target = NULL;
	m_job    = NULL;

	// make socket non-blocking
// FIXME -- check for error
	ARCH->setBlockingOnSocket(m_socket, false);

	// turn off Nagle algorithm.  we send lots of very short messages
	// that should be sent without (much) delay.  for example, the
	// mouse motion messages are much less useful if they're delayed.
// FIXME -- the client should do this
	try {
		ARCH->setNoDelayOnSocket(m_socket, true);
	}
	catch (XArchNetwork& e) {
		try {
			ARCH->closeSocket(m_socket);
			m_socket = NULL;
		}
		catch (XArchNetwork&) {
			// ignore
		}
		throw XSocketCreate(e.what());
	}
}

ISocketMultiplexerJob*
CTCPSocket::newMultiplexerJob(JobFunc func, bool readable, bool writable)
{
	return new TSocketMultiplexerMethodJob<CTCPSocket>(
								this, func, m_socket, readable, writable);
}

ISocketMultiplexerJob*
CTCPSocket::setState(State state, bool setJob)
{
	if (m_state == state || m_state == kClosed) {
		return m_job;
	}

	State oldState = m_state;
	m_state = state;

	bool read              = (m_input->getSize()  > 0);
	bool write             = (m_output->getSize() > 0);
	CEvent::Type eventType = 0;
	m_job                  = NULL;
	switch (m_state) {
	case kUnconnected:
		assert(0 && "cannot re-enter unconnected state");
		break;

	case kConnecting:
		m_job = newMultiplexerJob(&CTCPSocket::serviceConnecting, false, true);
		break;

	case kReadWrite:
		if (oldState == kConnecting) {
			eventType = IDataSocket::getConnectedEvent();
		}
		m_job = newMultiplexerJob(&CTCPSocket::serviceConnected, true, write);
		break;

	case kReadOnly:
		if (!write) {
			eventType = IDataSocket::getShutdownOutputEvent();
		}
		if (oldState == kWriteOnly) {
			goto shutdown;
		}
		m_job = newMultiplexerJob(&CTCPSocket::serviceConnected, true, write);
		break;

	case kWriteOnly:
		if (!read) {
			m_input->hangup();
			eventType = IDataSocket::getShutdownInputEvent();
		}
		if (oldState == kReadOnly) {
			goto shutdown;
		}
		m_job = newMultiplexerJob(&CTCPSocket::serviceConnected, false, write);
		break;

	case kShutdown:
shutdown:
		if (!read && !write) {
			eventType = ISocket::getDisconnectedEvent();
			m_state   = kClosed;
		}
		else {
			m_state   = kShutdown;
		}
		break;

	case kClosed:
		m_input->hangup();
		if (oldState == kConnecting) {
			eventType = IDataSocket::getConnectionFailedEvent();
		}
		else {
			eventType = ISocket::getDisconnectedEvent();
		}
		break;
	}

	// notify
	if (eventType != 0) {
		sendEvent(eventType);
	}

	// cut over to new job.  multiplexer will delete the old job.
	if (setJob) {
		if (m_job == NULL) {
			CSocketMultiplexer::getInstance()->removeSocket(this);
		}
		else {
			CSocketMultiplexer::getInstance()->addSocket(this, m_job);
		}
	}
	return m_job;
}

void
CTCPSocket::closeInput(void*)
{
	// note -- m_mutex should already be locked
	try {
		ARCH->closeSocketForRead(m_socket);
		setState(kWriteOnly, true);
	}
	catch (XArchNetwork&) {
		// ignore
	}
}

void
CTCPSocket::closeOutput(void*)
{
	// note -- m_mutex should already be locked
	try {
//		ARCH->closeSocketForWrite(m_socket);
		setState(kReadOnly, true);
	}
	catch (XArchNetwork&) {
		// ignore
	}
}

void
CTCPSocket::emptyInput(void*)
{
	// note -- m_mutex should already be locked
	bool write = (m_output->getSize() > 0);
	if (m_state == kWriteOnly && !write) {
		m_state = kShutdown;
	}
	if (m_state == kWriteOnly) {
		m_job = newMultiplexerJob(&CTCPSocket::serviceConnected, false, write);
		CSocketMultiplexer::getInstance()->addSocket(this, m_job);
		m_input->hangup();
		sendEvent(IDataSocket::getShutdownInputEvent());
	}
	else if (m_state == kShutdown) {
		m_job = NULL;
		CSocketMultiplexer::getInstance()->removeSocket(this);
		if (!write) {
			sendEvent(ISocket::getDisconnectedEvent());
			m_state = kClosed;
		}
	}
}

void
CTCPSocket::fillOutput(void*)
{
	// note -- m_mutex should already be locked
	if (m_state == kReadWrite) {
		m_job = newMultiplexerJob(&CTCPSocket::serviceConnected, true, true);
		CSocketMultiplexer::getInstance()->addSocket(this, m_job);
	}
	else if (m_state == kWriteOnly) {
		m_job = newMultiplexerJob(&CTCPSocket::serviceConnected, false, true);
		CSocketMultiplexer::getInstance()->addSocket(this, m_job);
	}
}

ISocketMultiplexerJob*
CTCPSocket::serviceConnecting(ISocketMultiplexerJob* job,
				bool, bool write, bool error)
{
	CLock lock(m_mutex);

	if (write && !error) {
		try {
			// connection may have failed or succeeded
			ARCH->throwErrorOnSocket(m_socket);
		}
		catch (XArchNetwork&) {
			error = true;
		}
	}

	if (error) {
		return setState(kClosed, false);
	}

	if (write) {
		return setState(kReadWrite, false);
	}

	return job;
}

ISocketMultiplexerJob*
CTCPSocket::serviceConnected(ISocketMultiplexerJob* job,
				bool read, bool write, bool error)
{
	CLock lock(m_mutex);
	if (error) {
		return setState(kClosed, false);
	}

	if (write) {
		// get amount of data to write
		UInt32 n = m_output->getSize();

		// write data
		try {
			const void* buffer = m_output->peek(n);
			size_t n2 = ARCH->writeSocket(m_socket, buffer, n);

			// discard written data
			if (n2 > 0) {
				m_output->pop(n2);
			}
		}
		catch (XArchNetworkDisconnected&) {
			// stream hungup
			return setState(kReadOnly, false);
		}
	}

	if (read) {
		UInt8 buffer[4096];
		size_t n = ARCH->readSocket(m_socket, buffer, sizeof(buffer));
		if (n > 0) {
			// slurp up as much as possible
			do {
				m_input->write(buffer, n);
				try {
					n = ARCH->readSocket(m_socket, buffer, sizeof(buffer));
				}
				catch (XArchNetworkWouldBlock&) {
					break;
				}
			} while (n > 0);

			// notify
			sendEvent(IDataSocket::getInputEvent());
		}
		else {
			// stream hungup
			return setState(kWriteOnly, false);
		}
	}

	if (write && m_output->getSize() == 0) {
		if (m_state == kReadOnly) {
			ARCH->closeSocketForWrite(m_socket);
			sendEvent(IDataSocket::getShutdownOutputEvent());
			m_job = newMultiplexerJob(&CTCPSocket::serviceConnected,
							true, false);
			job   = m_job;
		}
		else if (m_state == kReadWrite || m_state == kReadOnly) {
			m_job = newMultiplexerJob(&CTCPSocket::serviceConnected,
							true, false);
			job   = m_job;
		}
		else if (m_state == kWriteOnly) {
			m_job = NULL;
			job   = m_job;
		}
	}

	return job;
}

void
CTCPSocket::sendEvent(CEvent::Type type)
{
	CEventQueue::getInstance()->addEvent(CEvent(type, m_target, NULL));
}

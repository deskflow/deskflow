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
#include "XSocket.h"
#include "CLock.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "IEventJob.h"
#include "CArch.h"
#include "XArch.h"
#include <cstring>
#include <cstdlib>
#include <memory>

//
// CTCPSocket
//

CTCPSocket::CTCPSocket() :
	m_mutex(),
	m_flushed(&m_mutex, true)
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
	m_mutex(),
	m_socket(socket),
	m_flushed(&m_mutex, true)
{
	assert(m_socket != NULL);

	// socket starts in connected state
	init();
	onConnected();
	setJob(newJob());
}

CTCPSocket::~CTCPSocket()
{
	try {
		close();
	}
	catch (...) {
		// ignore
	}
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
	// remove ourself from the multiplexer
	setJob(NULL);

	CLock lock(&m_mutex);

	// clear buffers and enter disconnected state
	if (m_connected) {
		sendEvent(getDisconnectedEvent());
	}
	onDisconnected();

	// close the socket
	if (m_socket != NULL) {
		CArchSocket socket = m_socket;
		m_socket = NULL;
		try {
			ARCH->closeSocket(socket);
		}
		catch (XArchNetwork& e) {
			// ignore, there's not much we can do
			LOG((CLOG_WARN "error closing socket: %s", e.what().c_str()));
		}
	}
}

void*
CTCPSocket::getEventTarget() const
{
	return const_cast<void*>(reinterpret_cast<const void*>(this));
}

UInt32
CTCPSocket::read(void* buffer, UInt32 n)
{
	// copy data directly from our input buffer
	CLock lock(&m_mutex);
	UInt32 size = m_inputBuffer.getSize();
	if (n > size) {
		n = size;
	}
	if (buffer != NULL && n != 0) {
		memcpy(buffer, m_inputBuffer.peek(n), n);
	}
	m_inputBuffer.pop(n);

	// if no more data and we cannot read or write then send disconnected
	if (n > 0 && m_inputBuffer.getSize() == 0 && !m_readable && !m_writable) {
		sendEvent(getDisconnectedEvent());
		m_connected = false;
	}

	return n;
}

void
CTCPSocket::write(const void* buffer, UInt32 n)
{
	bool wasEmpty;
	{
		CLock lock(&m_mutex);

		// must not have shutdown output
		if (!m_writable) {
			sendEvent(getOutputErrorEvent());
			return;
		}

		// ignore empty writes
		if (n == 0) {
			return;
		}

		// copy data to the output buffer
		wasEmpty = (m_outputBuffer.getSize() == 0);
		m_outputBuffer.write(buffer, n);

		// there's data to write
		m_flushed = false;
	}

	// make sure we're waiting to write
	if (wasEmpty) {
		setJob(newJob());
	}
}

void
CTCPSocket::flush()
{
	CLock lock(&m_mutex);
	while (m_flushed == false) {
		m_flushed.wait();
	}
}

void
CTCPSocket::shutdownInput()
{
	bool useNewJob = false;
	{
		CLock lock(&m_mutex);

		// shutdown socket for reading
		try {
			ARCH->closeSocketForRead(m_socket);
		}
		catch (XArchNetwork&) {
			// ignore
		}

		// shutdown buffer for reading
		if (m_readable) {
			sendEvent(getInputShutdownEvent());
			onInputShutdown();
			useNewJob = true;
		}
	}
	if (useNewJob) {
		setJob(newJob());
	}
}

void
CTCPSocket::shutdownOutput()
{
	bool useNewJob = false;
	{
		CLock lock(&m_mutex);

		// shutdown socket for writing
		try {
			ARCH->closeSocketForWrite(m_socket);
		}
		catch (XArchNetwork&) {
			// ignore
		}

		// shutdown buffer for writing
		if (m_writable) {
			sendEvent(getOutputShutdownEvent());
			onOutputShutdown();
			useNewJob = true;
		}
	}
	if (useNewJob) {
		setJob(newJob());
	}
}

bool
CTCPSocket::isReady() const
{
	CLock lock(&m_mutex);
	return (m_inputBuffer.getSize() > 0);
}

UInt32
CTCPSocket::getSize() const
{
	CLock lock(&m_mutex);
	return m_inputBuffer.getSize();
}

void
CTCPSocket::connect(const CNetworkAddress& addr)
{
	{
		CLock lock(&m_mutex);

		// fail on attempts to reconnect
		if (m_socket == NULL || m_connected) {
			sendConnectionFailedEvent("busy");
			return;
		}

		try {
			if (ARCH->connectSocket(m_socket, addr.getAddress())) {
				sendEvent(getConnectedEvent());
				onConnected();
			}
			else {
				// connection is in progress
				m_writable = true;
			}
		}
		catch (XArchNetwork& e) {
			throw XSocketConnect(e.what());
		}
	}
	setJob(newJob());
}

void
CTCPSocket::init()
{
	// default state
	m_connected = false;
	m_readable  = false;
	m_writable  = false;

	try {
		// turn off Nagle algorithm.  we send lots of very short messages
		// that should be sent without (much) delay.  for example, the
		// mouse motion messages are much less useful if they're delayed.
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

void
CTCPSocket::setJob(ISocketMultiplexerJob* job)
{
	// multiplexer will delete the old job
	if (job == NULL) {
		CSocketMultiplexer::getInstance()->removeSocket(this);
	}
	else {
		CSocketMultiplexer::getInstance()->addSocket(this, job);
	}
}

ISocketMultiplexerJob*
CTCPSocket::newJob()
{
	// note -- must have m_mutex locked on entry

	if (m_socket == NULL) {
		return NULL;
	}
	else if (!m_connected) {
		assert(!m_readable);
		if (!(m_readable || m_writable)) {
			return NULL;
		}
		return new TSocketMultiplexerMethodJob<CTCPSocket>(
								this, &CTCPSocket::serviceConnecting,
								m_socket, m_readable, m_writable);
	}
	else {
		if (!(m_readable || (m_writable && (m_outputBuffer.getSize() > 0)))) {
			return NULL;
		}
		return new TSocketMultiplexerMethodJob<CTCPSocket>(
								this, &CTCPSocket::serviceConnected,
								m_socket, m_readable,
								m_writable && (m_outputBuffer.getSize() > 0));
	}
}

void
CTCPSocket::sendConnectionFailedEvent(const char* msg)
{
	CConnectionFailedInfo* info = (CConnectionFailedInfo*)malloc(
							sizeof(CConnectionFailedInfo) + strlen(msg));
	strcpy(info->m_what, msg);
	EVENTQUEUE->addEvent(CEvent(getConnectionFailedEvent(),
							getEventTarget(), info));
}

void
CTCPSocket::sendEvent(CEvent::Type type)
{
	EVENTQUEUE->addEvent(CEvent(type, getEventTarget(), NULL));
}

void
CTCPSocket::onConnected()
{
	m_connected = true;
	m_readable  = true;
	m_writable  = true;
}

void
CTCPSocket::onInputShutdown()
{
	m_inputBuffer.pop(m_inputBuffer.getSize());
	m_readable = false;
}

void
CTCPSocket::onOutputShutdown()
{
	m_outputBuffer.pop(m_outputBuffer.getSize());
	m_writable = false;

	// we're now flushed
	m_flushed = true;
	m_flushed.broadcast();
}

void
CTCPSocket::onDisconnected()
{
	// disconnected
	onInputShutdown();
	onOutputShutdown();
	m_connected = false;
}

ISocketMultiplexerJob*
CTCPSocket::serviceConnecting(ISocketMultiplexerJob* job,
				bool, bool write, bool error)
{
	CLock lock(&m_mutex);

	// should only check for errors if error is true but checking a new
	// socket (and a socket that's connecting should be new) for errors
	// should be safe and Mac OS X appears to have a bug where a
	// non-blocking stream socket that fails to connect immediately is
	// reported by select as being writable (i.e. connected) even when
	// the connection has failed.  this is easily demonstrated on OS X
	// 10.3.4 by starting a synergy client and telling to connect to
	// another system that's not running a synergy server.  it will
	// claim to have connected then quickly disconnect (i guess because
	// read returns 0 bytes).  unfortunately, synergy attempts to
	// reconnect immediately, the process repeats and we end up
	// spinning the CPU.  luckily, OS X does set SO_ERROR on the
	// socket correctly when the connection has failed so checking for
	// errors works.  (curiously, sometimes OS X doesn't report
	// connection refused.  when that happens it at least doesn't
	// report the socket as being writable so synergy is able to time
	// out the attempt.)
	if (error || true) {
		try {
			// connection may have failed or succeeded
			ARCH->throwErrorOnSocket(m_socket);
		}
		catch (XArchNetwork& e) {
			sendConnectionFailedEvent(e.what().c_str());
			onDisconnected();
			return newJob();
		}
	}

	if (write) {
		sendEvent(getConnectedEvent());
		onConnected();
		return newJob();
	}

	return job;
}

ISocketMultiplexerJob*
CTCPSocket::serviceConnected(ISocketMultiplexerJob* job,
				bool read, bool write, bool error)
{
	CLock lock(&m_mutex);

	if (error) {
		sendEvent(getDisconnectedEvent());
		onDisconnected();
		return newJob();
	}

	bool needNewJob = false;

	if (write) {
		try {
			// write data
			UInt32 n = m_outputBuffer.getSize();
			const void* buffer = m_outputBuffer.peek(n);
			n = (UInt32)ARCH->writeSocket(m_socket, buffer, n);

			// discard written data
			if (n > 0) {
				m_outputBuffer.pop(n);
				if (m_outputBuffer.getSize() == 0) {
					sendEvent(getOutputFlushedEvent());
					m_flushed = true;
					m_flushed.broadcast();
					needNewJob = true;
				}
			}
		}
		catch (XArchNetworkShutdown&) {
			// remote read end of stream hungup.  our output side
			// has therefore shutdown.
			onOutputShutdown();
			sendEvent(getOutputShutdownEvent());
			if (!m_readable && m_inputBuffer.getSize() == 0) {
				sendEvent(getDisconnectedEvent());
				m_connected = false;
			}
			needNewJob = true;
		}
		catch (XArchNetworkDisconnected&) {
			// stream hungup
			onDisconnected();
			sendEvent(getDisconnectedEvent());
			needNewJob = true;
		}
		catch (XArchNetwork& e) {
			// other write error
			LOG((CLOG_WARN "error writing socket: %s", e.what().c_str()));
			onDisconnected();
			sendEvent(getOutputErrorEvent());
			sendEvent(getDisconnectedEvent());
			needNewJob = true;
		}
	}

	if (read && m_readable) {
		try {
			UInt8 buffer[4096];
			size_t n = ARCH->readSocket(m_socket, buffer, sizeof(buffer));
			if (n > 0) {
				bool wasEmpty = (m_inputBuffer.getSize() == 0);

				// slurp up as much as possible
				do {
					m_inputBuffer.write(buffer, n);
					n = ARCH->readSocket(m_socket, buffer, sizeof(buffer));
				} while (n > 0);

				// send input ready if input buffer was empty
				if (wasEmpty) {
					sendEvent(getInputReadyEvent());
				}
			}
			else {
				// remote write end of stream hungup.  our input side
				// has therefore shutdown but don't flush our buffer
				// since there's still data to be read.
				sendEvent(getInputShutdownEvent());
				if (!m_writable && m_inputBuffer.getSize() == 0) {
					sendEvent(getDisconnectedEvent());
					m_connected = false;
				}
				m_readable = false;
				needNewJob = true;
			}
		}
		catch (XArchNetworkDisconnected&) {
			// stream hungup
			sendEvent(getDisconnectedEvent());
			onDisconnected();
			needNewJob = true;
		}
		catch (XArchNetwork& e) {
			// ignore other read error
			LOG((CLOG_WARN "error reading socket: %s", e.what().c_str()));
		}
	}

	return needNewJob ? newJob() : job;
}

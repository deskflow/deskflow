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
#include "CBufferedInputStream.h"
#include "CBufferedOutputStream.h"
#include "CNetworkAddress.h"
#include "XIO.h"
#include "XSocket.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
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
	if (m_socket != NULL) {
		CLock lock(m_mutex);
		try {
			ARCH->closeSocketForRead(m_socket);
		}
		catch (XArchNetwork&) {
			// ignore
		}
		try {
			ARCH->closeSocketForWrite(m_socket);
		}
		catch (XArchNetwork&) {
			// ignore
		}
		m_connected = kClosed;
	}

	// wait for thread
	if (m_thread != NULL) {
		m_thread->wait();
		delete m_thread;
		m_thread = NULL;
	}

	// close socket
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
CTCPSocket::connect(const CNetworkAddress& addr)
{
	do {
		// connect asynchronously so we can check for cancellation.
		// we can't wrap setting and resetting the blocking flag in
		// the c'tor/d'tor of a class (to make resetting automatic)
		// because setBlockingOnSocket() can throw and it might be
		// called while unwinding the stack due to a throw.
		try {
			ARCH->setBlockingOnSocket(m_socket, false);
			ARCH->connectSocket(m_socket, addr.getAddress());
			ARCH->setBlockingOnSocket(m_socket, true);

			// connected
			break;
		}
		catch (XArchNetworkConnecting&) {
			// connection is in progress
			ARCH->setBlockingOnSocket(m_socket, true);
		}
		catch (XArchNetwork& e) {
			ARCH->setBlockingOnSocket(m_socket, true);
			throw XSocketConnect(e.what());
		}

		// wait for connection or failure
		IArchNetwork::CPollEntry pfds[1];
		pfds[0].m_socket = m_socket;
		pfds[0].m_events = IArchNetwork::kPOLLOUT;
		for (;;) {
			ARCH->testCancelThread();
			try {
				const int status = ARCH->pollSocket(pfds, 1, 0.01);
				if (status > 0) {
					if ((pfds[0].m_revents & (IArchNetwork::kPOLLERR |
											  IArchNetwork::kPOLLNVAL)) != 0) {
						// connection failed
						ARCH->throwErrorOnSocket(m_socket);
					}
					if ((pfds[0].m_revents & IArchNetwork::kPOLLOUT) != 0) {
						// connection may have failed or succeeded
						ARCH->throwErrorOnSocket(m_socket);

						// connected!
						break;
					}
				}
			}
			catch (XArchNetwork& e) {
				throw XSocketConnect(e.what());
			}
		}
	} while (false);

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
	assert(m_socket != NULL);

	// now service the connection
	IArchNetwork::CPollEntry pfds[1];
	pfds[0].m_socket = m_socket;
	for (;;) {
		{
			// choose events to poll for
			CLock lock(m_mutex);
			pfds[0].m_events = 0;
			if (m_connected == 0) {
				return;
			}
			if ((m_connected & kRead) != 0) {
				// still open for reading
				pfds[0].m_events |= IArchNetwork::kPOLLIN;
			}
			if ((m_connected & kWrite) != 0 && m_output->getSize() > 0) {
				// data queued for writing
				pfds[0].m_events |= IArchNetwork::kPOLLOUT;
			}
		}

		try {
			// check for status
			const int status = ARCH->pollSocket(pfds, 1, 0.01);

			// transfer data and handle errors
			if (status == 1) {
				if ((pfds[0].m_revents & (IArchNetwork::kPOLLERR |
										  IArchNetwork::kPOLLNVAL)) != 0) {
					// stream is no good anymore so bail
					CLock lock(m_mutex);
					m_input->hangup();
					return;
				}

				// read some data
				if (pfds[0].m_revents & IArchNetwork::kPOLLIN) {
					UInt8 buffer[4096];
					size_t n = ARCH->readSocket(m_socket,
												buffer, sizeof(buffer));
					CLock lock(m_mutex);
					if (n > 0) {
						m_input->write(buffer, n);
					}
					else {
						// stream hungup
						m_input->hangup();
						m_connected &= ~kRead;
					}
				}

				// write some data
				if (pfds[0].m_revents & IArchNetwork::kPOLLOUT) {
					CLock lock(m_mutex);

					// get amount of data to write
					UInt32 n = m_output->getSize();

					// write data
					const void* buffer = m_output->peek(n);
					size_t n2 = ARCH->writeSocket(m_socket, buffer, n);

					// discard written data
					if (n2 > 0) {
						m_output->pop(n2);
					}
				}
			}
		}
		catch (XArchNetwork&) {
			// socket has failed
			return;
		}
	}
}

void
CTCPSocket::closeInput(void*)
{
	// note -- m_mutex should already be locked
	try {
		ARCH->closeSocketForRead(m_socket);
		m_connected &= ~kRead;
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
		ARCH->closeSocketForWrite(m_socket);
		m_connected &= ~kWrite;
	}
	catch (XArchNetwork&) {
		// ignore
	}
}

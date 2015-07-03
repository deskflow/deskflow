/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "net/IDataSocket.h"
#include "io/StreamBuffer.h"
#include "mt/CondVar.h"
#include "mt/Mutex.h"
#include "arch/IArchNetwork.h"

class Mutex;
class Thread;
class ISocketMultiplexerJob;
class IEventQueue;
class SocketMultiplexer;

//! TCP data socket
/*!
A data socket using TCP.
*/
class TCPSocket : public IDataSocket {
public:
	TCPSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer);
	TCPSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, ArchSocket socket);
	virtual ~TCPSocket();

	// ISocket overrides
	virtual void		bind(const NetworkAddress&);
	virtual void		close();
	virtual void*		getEventTarget() const;

	// IStream overrides
	virtual UInt32		read(void* buffer, UInt32 n);
	virtual void		write(const void* buffer, UInt32 n);
	virtual void		flush();
	virtual void		shutdownInput();
	virtual void		shutdownOutput();
	virtual bool		isReady() const;
	virtual bool		isFatal() const;
	virtual UInt32		getSize() const;

	// IDataSocket overrides
	virtual void		connect(const NetworkAddress&);

	virtual void		secureConnect() {}
	virtual void		secureAccept() {}
	virtual void		setFingerprintFilename(String& f) {}

protected:
	ArchSocket			getSocket() { return m_socket; }
	IEventQueue*		getEvents() { return m_events; }
	virtual bool		isSecureReady() { return false; }
	virtual bool		isSecure() { return false; }
	virtual int			secureRead(void* buffer, int, int& ) { return 0; }
	virtual int			secureWrite(const void*, int, int& ) { return 0; }

	void				setJob(ISocketMultiplexerJob*);
	ISocketMultiplexerJob*
						newJob();
	bool				isReadable() { return m_readable; }
	bool				isWritable() { return m_writable; }

	Mutex&				getMutex() { return m_mutex; }

	void				sendEvent(Event::Type);

private:
	void				init();

	void				sendConnectionFailedEvent(const char*);
	void				onConnected();
	void				onInputShutdown();
	void				onOutputShutdown();
	void				onDisconnected();

	ISocketMultiplexerJob*
						serviceConnecting(ISocketMultiplexerJob*,
							bool, bool, bool);
	ISocketMultiplexerJob*
						serviceConnected(ISocketMultiplexerJob*,
							bool, bool, bool);

protected:
	bool				m_readable;
	bool				m_writable;

private:
	Mutex				m_mutex;
	ArchSocket			m_socket;
	StreamBuffer		m_inputBuffer;
	StreamBuffer		m_outputBuffer;
	CondVar<bool>		m_flushed;
	bool				m_connected;
	IEventQueue*		m_events;
	SocketMultiplexer*	m_socketMultiplexer;
};

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CTCPSOCKET_H
#define CTCPSOCKET_H

#include "IDataSocket.h"
#include "CStreamBuffer.h"
#include "CCondVar.h"
#include "CMutex.h"
#include "IArchNetwork.h"

class CMutex;
class CThread;
class ISocketMultiplexerJob;
class IEventQueue;
class CSocketMultiplexer;

//! TCP data socket
/*!
A data socket using TCP.
*/
class CTCPSocket : public IDataSocket {
public:
	CTCPSocket(IEventQueue* events, CSocketMultiplexer* socketMultiplexer);
	CTCPSocket(IEventQueue* events, CSocketMultiplexer* socketMultiplexer, CArchSocket socket);
	~CTCPSocket();

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&);
	virtual void		close();
	virtual void*		getEventTarget() const;

	// IStream overrides
	virtual UInt32		read(void* buffer, UInt32 n);
	virtual void		write(const void* buffer, UInt32 n);
	virtual void		flush();
	virtual void		shutdownInput();
	virtual void		shutdownOutput();
	virtual bool		isReady() const;
	virtual UInt32		getSize() const;

	// IDataSocket overrides
	virtual void		connect(const CNetworkAddress&);

private:
	void				init();

	void				setJob(ISocketMultiplexerJob*);
	ISocketMultiplexerJob*	newJob();
	void				sendConnectionFailedEvent(const char*);
	void				sendEvent(CEvent::Type);

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

private:
	CMutex				m_mutex;
	CArchSocket			m_socket;
	CStreamBuffer		m_inputBuffer;
	CStreamBuffer		m_outputBuffer;
	CCondVar<bool>		m_flushed;
	bool				m_connected;
	bool				m_readable;
	bool				m_writable;
	IEventQueue*		m_events;
	CSocketMultiplexer* m_socketMultiplexer;
};

#endif

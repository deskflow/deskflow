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

#ifndef CTCPSOCKET_H
#define CTCPSOCKET_H

#include "IDataSocket.h"
#include "CEvent.h"
#include "BasicTypes.h"
#include "IArchNetwork.h"

class CMutex;
class CThread;
class CBufferedInputStream;
class CBufferedOutputStream;
class ISocketMultiplexerJob;

//! TCP data socket
/*!
A data socket using TCP.
*/
class CTCPSocket : public IDataSocket {
public:
	CTCPSocket();
	CTCPSocket(CArchSocket);
	~CTCPSocket();

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&);
	virtual void		close();
	virtual void		setEventTarget(void*);

	// IDataSocket overrides
	virtual void		connect(const CNetworkAddress&);
	virtual IInputStream*	getInputStream();
	virtual IOutputStream*	getOutputStream();

private:
	enum State {
		kUnconnected,
		kConnecting,
		kReadWrite,
		kReadOnly,
		kWriteOnly,
		kShutdown,
		kClosed
	};

	void				init();

	ISocketMultiplexerJob*
						setState(State, bool setJob);

	void				closeInput(void*);
	void				closeOutput(void*);
	void				emptyInput(void*);
	void				fillOutput(void*);

	ISocketMultiplexerJob*
						serviceConnecting(ISocketMultiplexerJob*,
							bool, bool, bool);
	ISocketMultiplexerJob*
						serviceConnected(ISocketMultiplexerJob*,
							bool, bool, bool);

	typedef ISocketMultiplexerJob* (CTCPSocket::*JobFunc)(
							ISocketMultiplexerJob*,
							bool, bool, bool);
	ISocketMultiplexerJob*
						newMultiplexerJob(JobFunc,
							bool readable, bool writable);

	void				sendEvent(CEvent::Type);

private:
	CArchSocket				m_socket;
	CBufferedInputStream*	m_input;
	CBufferedOutputStream*	m_output;

	CMutex*				m_mutex;
	State				m_state;
	void*				m_target;

	ISocketMultiplexerJob*	m_job;
};

#endif

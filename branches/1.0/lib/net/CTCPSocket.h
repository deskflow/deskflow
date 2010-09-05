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
#include "BasicTypes.h"
#include "IArchNetwork.h"

class CMutex;
class CThread;
class CBufferedInputStream;
class CBufferedOutputStream;

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

	// IDataSocket overrides
	virtual void		connect(const CNetworkAddress&);
	virtual IInputStream*	getInputStream();
	virtual IOutputStream*	getOutputStream();

private:
	void				init();
	void				ioThread(void*);
	void				ioCleanup();
	void				ioService();
	void				closeInput(void*);
	void				closeOutput(void*);

private:
	enum { kClosed = 0, kRead = 1, kWrite = 2, kReadWrite = 3 };

	CArchSocket				m_socket;
	CBufferedInputStream*	m_input;
	CBufferedOutputStream*	m_output;

	CMutex*				m_mutex;
	CThread*			m_thread;
	UInt32				m_connected;
};

#endif

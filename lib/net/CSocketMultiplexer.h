/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#ifndef CSOCKETMULTIPLEXER_H
#define CSOCKETMULTIPLEXER_H

#include "IArchNetwork.h"
#include "stdlist.h"
#include "stdmap.h"

template <class T>
class CCondVar;
class CMutex;
class CThread;
class ISocket;
class ISocketMultiplexerJob;

//! Socket multiplexer
/*!
A socket multiplexer services multiple sockets simultaneously.
*/
class CSocketMultiplexer {
public:
	CSocketMultiplexer();
	~CSocketMultiplexer();

	//! @name manipulators
	//@{

	void				addSocket(ISocket*, ISocketMultiplexerJob*);

	void				removeSocket(ISocket*);

	//@}
	//! @name accessors
	//@{

	// maybe belongs on ISocketMultiplexer
	static CSocketMultiplexer*
						getInstance();

	//@}

private:
	// list of jobs.  we use a list so we can safely iterate over it
	// while other threads modify it.
	typedef std::list<ISocketMultiplexerJob*> CSocketJobs;
	typedef CSocketJobs::iterator CJobCursor;
	typedef std::map<ISocket*, CJobCursor> CSocketJobMap;

	// service sockets.  the service thread will only access m_sockets
	// and m_update while m_pollable and m_polling are true.  all other
	// threads must only modify these when m_pollable and m_polling are
	// false.  only the service thread sets m_polling.
	void				serviceThread(void*);

	// create, iterate, and destroy a cursor.  a cursor is used to
	// safely iterate through the job list while other threads modify
	// the list.  it works by inserting a dummy item in the list and
	// moving that item through the list.  the dummy item will never
	// be removed by other edits so an iterator pointing at the item
	// remains valid until we remove the dummy item in deleteCursor().
	// nextCursor() finds the next non-dummy item, moves our dummy
	// item just past it, and returns an iterator for the non-dummy
	// item.  all cursor calls lock the mutex for their duration.
	CJobCursor			newCursor();
	CJobCursor			nextCursor(CJobCursor);
	void				deleteCursor(CJobCursor);

private:
	CMutex*				m_mutex;
	CCondVar<bool>*		m_pollable;
	CCondVar<bool>*		m_polling;
	CThread*			m_thread;
	bool				m_update;

	CSocketJobs			m_socketJobs;
	CSocketJobMap		m_socketJobMap;
	ISocketMultiplexerJob*	m_cursorMark;

	static CSocketMultiplexer*	s_instance;
};

#endif

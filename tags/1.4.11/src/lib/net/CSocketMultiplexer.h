/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

	// lock out locking the job list.  this blocks if another thread
	// has already locked out locking.  once it returns, only the
	// calling thread will be able to lock the job list after any
	// current lock is released.
	void				lockJobListLock();

	// lock the job list.  this blocks if the job list is already
	// locked.  the calling thread must have called requestJobLock.
	void				lockJobList();

	// unlock the job list and the lock out on locking.
	void				unlockJobList();

private:
	CMutex*				m_mutex;
	CThread*			m_thread;
	bool				m_update;
	CCondVar<bool>*		m_jobsReady;
	CCondVar<bool>*		m_jobListLock;
	CCondVar<bool>*		m_jobListLockLocked;
	CThread*			m_jobListLocker;
	CThread*			m_jobListLockLocker;

	CSocketJobs			m_socketJobs;
	CSocketJobMap		m_socketJobMap;
	ISocketMultiplexerJob*	m_cursorMark;

	static CSocketMultiplexer*	s_instance;
};

#endif

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "arch/IArchNetwork.h"
#include "common/stdlist.h"
#include "common/stdmap.h"

template <class T>
class CondVar;
class Mutex;
class Thread;
class ISocket;
class ISocketMultiplexerJob;

//! Socket multiplexer
/*!
A socket multiplexer services multiple sockets simultaneously.
*/
class SocketMultiplexer {
public:
    SocketMultiplexer ();
    ~SocketMultiplexer ();

    //! @name manipulators
    //@{

    void addSocket (ISocket*, ISocketMultiplexerJob*);

    void removeSocket (ISocket*);

    //@}
    //! @name accessors
    //@{

    // maybe belongs on ISocketMultiplexer
    static SocketMultiplexer* getInstance ();

    //@}

private:
    // list of jobs.  we use a list so we can safely iterate over it
    // while other threads modify it.
    typedef std::list<ISocketMultiplexerJob*> SocketJobs;
    typedef SocketJobs::iterator JobCursor;
    typedef std::map<ISocket*, JobCursor> SocketJobMap;

    // service sockets.  the service thread will only access m_sockets
    // and m_update while m_pollable and m_polling are true.  all other
    // threads must only modify these when m_pollable and m_polling are
    // false.  only the service thread sets m_polling.
    void serviceThread (void*);

    // create, iterate, and destroy a cursor.  a cursor is used to
    // safely iterate through the job list while other threads modify
    // the list.  it works by inserting a dummy item in the list and
    // moving that item through the list.  the dummy item will never
    // be removed by other edits so an iterator pointing at the item
    // remains valid until we remove the dummy item in deleteCursor().
    // nextCursor() finds the next non-dummy item, moves our dummy
    // item just past it, and returns an iterator for the non-dummy
    // item.  all cursor calls lock the mutex for their duration.
    JobCursor newCursor ();
    JobCursor nextCursor (JobCursor);
    void deleteCursor (JobCursor);

    // lock out locking the job list.  this blocks if another thread
    // has already locked out locking.  once it returns, only the
    // calling thread will be able to lock the job list after any
    // current lock is released.
    void lockJobListLock ();

    // lock the job list.  this blocks if the job list is already
    // locked.  the calling thread must have called requestJobLock.
    void lockJobList ();

    // unlock the job list and the lock out on locking.
    void unlockJobList ();

private:
    Mutex* m_mutex;
    Thread* m_thread;
    bool m_update;
    CondVar<bool>* m_jobsReady;
    CondVar<bool>* m_jobListLock;
    CondVar<bool>* m_jobListLockLocked;
    Thread* m_jobListLocker;
    Thread* m_jobListLockLocker;

    SocketJobs m_socketJobs;
    SocketJobMap m_socketJobMap;
    ISocketMultiplexerJob* m_cursorMark;
};

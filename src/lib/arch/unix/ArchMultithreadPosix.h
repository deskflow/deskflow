/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#include "arch/IArchMultithread.h"
#include "common/stdlist.h"

#include <pthread.h>

#define ARCH_MULTITHREAD ArchMultithreadPosix

class ArchCondImpl {
public:
    pthread_cond_t m_cond;
};

class ArchMutexImpl {
public:
    pthread_mutex_t m_mutex;
};

//! Posix implementation of IArchMultithread
class ArchMultithreadPosix : public IArchMultithread {
public:
    ArchMultithreadPosix ();
    virtual ~ArchMultithreadPosix ();

    //! @name manipulators
    //@{

    void setNetworkDataForCurrentThread (void*);

    //@}
    //! @name accessors
    //@{

    void* getNetworkDataForThread (ArchThread);

    static ArchMultithreadPosix* getInstance ();

    //@}

    // IArchMultithread overrides
    virtual ArchCond newCondVar ();
    virtual void closeCondVar (ArchCond);
    virtual void signalCondVar (ArchCond);
    virtual void broadcastCondVar (ArchCond);
    virtual bool waitCondVar (ArchCond, ArchMutex, double timeout);
    virtual ArchMutex newMutex ();
    virtual void closeMutex (ArchMutex);
    virtual void lockMutex (ArchMutex);
    virtual void unlockMutex (ArchMutex);
    virtual ArchThread newThread (ThreadFunc, void*);
    virtual ArchThread newCurrentThread ();
    virtual ArchThread copyThread (ArchThread);
    virtual void closeThread (ArchThread);
    virtual void cancelThread (ArchThread);
    virtual void setPriorityOfThread (ArchThread, int n);
    virtual void testCancelThread ();
    virtual bool wait (ArchThread, double timeout);
    virtual bool isSameThread (ArchThread, ArchThread);
    virtual bool isExitedThread (ArchThread);
    virtual void* getResultOfThread (ArchThread);
    virtual ThreadID getIDOfThread (ArchThread);
    virtual void setSignalHandler (ESignal, SignalFunc, void*);
    virtual void raiseSignal (ESignal);

private:
    void startSignalHandler ();

    ArchThreadImpl* find (pthread_t thread);
    ArchThreadImpl* findNoRef (pthread_t thread);
    void insert (ArchThreadImpl* thread);
    void erase (ArchThreadImpl* thread);

    void refThread (ArchThreadImpl* rep);
    void testCancelThreadImpl (ArchThreadImpl* rep);

    void doThreadFunc (ArchThread thread);
    static void* threadFunc (void* vrep);
    static void threadCancel (int);
    static void* threadSignalHandler (void* vrep);

private:
    typedef std::list<ArchThread> ThreadList;

    static ArchMultithreadPosix* s_instance;

    bool m_newThreadCalled;

    ArchMutex m_threadMutex;
    ArchThread m_mainThread;
    ThreadList m_threadList;
    ThreadID m_nextID;

    pthread_t m_signalThread;
    SignalFunc m_signalFunc[kNUM_SIGNALS];
    void* m_signalUserData[kNUM_SIGNALS];
};

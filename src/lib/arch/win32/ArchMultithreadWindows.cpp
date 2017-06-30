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

#if defined(_MSC_VER) && !defined(_MT)
#error multithreading compile option is required
#endif

#include "arch/win32/ArchMultithreadWindows.h"
#include "arch/Arch.h"
#include "arch/XArch.h"

#include <process.h>

//
// note -- implementation of condition variable taken from:
//   http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
// titled "Strategies for Implementing POSIX Condition Variables
// on Win32."  it also provides an implementation that doesn't
// suffer from the incorrectness problem described in our
// corresponding header but it is slower, still unfair, and
// can cause busy waiting.
//

//
// ArchThreadImpl
//

class ArchThreadImpl {
public:
    ArchThreadImpl ();
    ~ArchThreadImpl ();

public:
    int m_refCount;
    HANDLE m_thread;
    DWORD m_id;
    IArchMultithread::ThreadFunc m_func;
    void* m_userData;
    HANDLE m_cancel;
    bool m_cancelling;
    HANDLE m_exit;
    void* m_result;
    void* m_networkData;
};

ArchThreadImpl::ArchThreadImpl ()
    : m_refCount (1),
      m_thread (NULL),
      m_id (0),
      m_func (NULL),
      m_userData (NULL),
      m_cancelling (false),
      m_result (NULL),
      m_networkData (NULL) {
    m_exit   = CreateEvent (NULL, TRUE, FALSE, NULL);
    m_cancel = CreateEvent (NULL, TRUE, FALSE, NULL);
}

ArchThreadImpl::~ArchThreadImpl () {
    CloseHandle (m_exit);
    CloseHandle (m_cancel);
}


//
// ArchMultithreadWindows
//

ArchMultithreadWindows* ArchMultithreadWindows::s_instance = NULL;

ArchMultithreadWindows::ArchMultithreadWindows () {
    assert (s_instance == NULL);
    s_instance = this;

    // no signal handlers
    for (size_t i = 0; i < kNUM_SIGNALS; ++i) {
        m_signalFunc[i]     = NULL;
        m_signalUserData[i] = NULL;
    }

    // create mutex for thread list
    m_threadMutex = newMutex ();

    // create thread for calling (main) thread and add it to our
    // list.  no need to lock the mutex since we're the only thread.
    m_mainThread           = new ArchThreadImpl;
    m_mainThread->m_thread = NULL;
    m_mainThread->m_id     = GetCurrentThreadId ();
    insert (m_mainThread);
}

ArchMultithreadWindows::~ArchMultithreadWindows () {
    s_instance = NULL;

    // clean up thread list
    for (ThreadList::iterator index = m_threadList.begin ();
         index != m_threadList.end ();
         ++index) {
        delete *index;
    }

    // done with mutex
    delete m_threadMutex;
}

void
ArchMultithreadWindows::setNetworkDataForCurrentThread (void* data) {
    lockMutex (m_threadMutex);
    ArchThreadImpl* thread = findNoRef (GetCurrentThreadId ());
    thread->m_networkData  = data;
    unlockMutex (m_threadMutex);
}

void*
ArchMultithreadWindows::getNetworkDataForThread (ArchThread thread) {
    lockMutex (m_threadMutex);
    void* data = thread->m_networkData;
    unlockMutex (m_threadMutex);
    return data;
}

HANDLE
ArchMultithreadWindows::getCancelEventForCurrentThread () {
    lockMutex (m_threadMutex);
    ArchThreadImpl* thread = findNoRef (GetCurrentThreadId ());
    unlockMutex (m_threadMutex);
    return thread->m_cancel;
}

ArchMultithreadWindows*
ArchMultithreadWindows::getInstance () {
    return s_instance;
}

ArchCond
ArchMultithreadWindows::newCondVar () {
    ArchCondImpl* cond = new ArchCondImpl;
    cond->m_events[ArchCondImpl::kSignal] =
        CreateEvent (NULL, FALSE, FALSE, NULL);
    cond->m_events[ArchCondImpl::kBroadcast] =
        CreateEvent (NULL, TRUE, FALSE, NULL);
    cond->m_waitCountMutex = newMutex ();
    cond->m_waitCount      = 0;
    return cond;
}

void
ArchMultithreadWindows::closeCondVar (ArchCond cond) {
    CloseHandle (cond->m_events[ArchCondImpl::kSignal]);
    CloseHandle (cond->m_events[ArchCondImpl::kBroadcast]);
    closeMutex (cond->m_waitCountMutex);
    delete cond;
}

void
ArchMultithreadWindows::signalCondVar (ArchCond cond) {
    // is anybody waiting?
    lockMutex (cond->m_waitCountMutex);
    const bool hasWaiter = (cond->m_waitCount > 0);
    unlockMutex (cond->m_waitCountMutex);

    // wake one thread if anybody is waiting
    if (hasWaiter) {
        SetEvent (cond->m_events[ArchCondImpl::kSignal]);
    }
}

void
ArchMultithreadWindows::broadcastCondVar (ArchCond cond) {
    // is anybody waiting?
    lockMutex (cond->m_waitCountMutex);
    const bool hasWaiter = (cond->m_waitCount > 0);
    unlockMutex (cond->m_waitCountMutex);

    // wake all threads if anybody is waiting
    if (hasWaiter) {
        SetEvent (cond->m_events[ArchCondImpl::kBroadcast]);
    }
}

bool
ArchMultithreadWindows::waitCondVar (ArchCond cond, ArchMutex mutex,
                                     double timeout) {
    // prepare to wait
    const DWORD winTimeout =
        (timeout < 0.0) ? INFINITE : static_cast<DWORD> (1000.0 * timeout);

    // make a list of the condition variable events and the cancel event
    // for the current thread.
    HANDLE handles[4];
    handles[0] = cond->m_events[ArchCondImpl::kSignal];
    handles[1] = cond->m_events[ArchCondImpl::kBroadcast];
    handles[2] = getCancelEventForCurrentThread ();

    // update waiter count
    lockMutex (cond->m_waitCountMutex);
    ++cond->m_waitCount;
    unlockMutex (cond->m_waitCountMutex);

    // release mutex.  this should be atomic with the wait so that it's
    // impossible for another thread to signal us between the unlock and
    // the wait, which would lead to a lost signal on broadcasts.
    // however, we're using a manual reset event for broadcasts which
    // stays set until we reset it, so we don't lose the broadcast.
    unlockMutex (mutex);

    // wait for a signal or broadcast
    DWORD result = WaitForMultipleObjects (3, handles, FALSE, winTimeout);

    // cancel takes priority
    if (result != WAIT_OBJECT_0 + 2 &&
        WaitForSingleObject (handles[2], 0) == WAIT_OBJECT_0) {
        result = WAIT_OBJECT_0 + 2;
    }

    // update the waiter count and check if we're the last waiter
    lockMutex (cond->m_waitCountMutex);
    --cond->m_waitCount;
    const bool last = (result == WAIT_OBJECT_0 + 1 && cond->m_waitCount == 0);
    unlockMutex (cond->m_waitCountMutex);

    // reset the broadcast event if we're the last waiter
    if (last) {
        ResetEvent (cond->m_events[ArchCondImpl::kBroadcast]);
    }

    // reacquire the mutex
    lockMutex (mutex);

    // cancel thread if necessary
    if (result == WAIT_OBJECT_0 + 2) {
        ARCH->testCancelThread ();
    }

    // return success or failure
    return (result == WAIT_OBJECT_0 + 0 || result == WAIT_OBJECT_0 + 1);
}

ArchMutex
ArchMultithreadWindows::newMutex () {
    ArchMutexImpl* mutex = new ArchMutexImpl;
    InitializeCriticalSection (&mutex->m_mutex);
    return mutex;
}

void
ArchMultithreadWindows::closeMutex (ArchMutex mutex) {
    DeleteCriticalSection (&mutex->m_mutex);
    delete mutex;
}

void
ArchMultithreadWindows::lockMutex (ArchMutex mutex) {
    EnterCriticalSection (&mutex->m_mutex);
}

void
ArchMultithreadWindows::unlockMutex (ArchMutex mutex) {
    LeaveCriticalSection (&mutex->m_mutex);
}

ArchThread
ArchMultithreadWindows::newThread (ThreadFunc func, void* data) {
    lockMutex (m_threadMutex);

    // create thread impl for new thread
    ArchThreadImpl* thread = new ArchThreadImpl;
    thread->m_func         = func;
    thread->m_userData     = data;

    // create thread
    unsigned int id  = 0;
    thread->m_thread = reinterpret_cast<HANDLE> (
        _beginthreadex (NULL, 0, threadFunc, (void*) thread, 0, &id));
    thread->m_id = static_cast<DWORD> (id);

    // check if thread was started
    if (thread->m_thread == 0) {
        // failed to start thread so clean up
        delete thread;
        thread = NULL;
    } else {
        // add thread to list
        insert (thread);

        // increment ref count to account for the thread itself
        refThread (thread);
    }

    // note that the child thread will wait until we release this mutex
    unlockMutex (m_threadMutex);

    return thread;
}

ArchThread
ArchMultithreadWindows::newCurrentThread () {
    lockMutex (m_threadMutex);
    ArchThreadImpl* thread = find (GetCurrentThreadId ());
    unlockMutex (m_threadMutex);
    assert (thread != NULL);
    return thread;
}

void
ArchMultithreadWindows::closeThread (ArchThread thread) {
    assert (thread != NULL);

    // decrement ref count and clean up thread if no more references
    if (--thread->m_refCount == 0) {
        // close the handle (main thread has a NULL handle)
        if (thread->m_thread != NULL) {
            CloseHandle (thread->m_thread);
        }

        // remove thread from list
        lockMutex (m_threadMutex);
        assert (findNoRefOrCreate (thread->m_id) == thread);
        erase (thread);
        unlockMutex (m_threadMutex);

        // done with thread
        delete thread;
    }
}

ArchThread
ArchMultithreadWindows::copyThread (ArchThread thread) {
    refThread (thread);
    return thread;
}

void
ArchMultithreadWindows::cancelThread (ArchThread thread) {
    assert (thread != NULL);

    // set cancel flag
    SetEvent (thread->m_cancel);
}

void
ArchMultithreadWindows::setPriorityOfThread (ArchThread thread, int n) {
    struct PriorityInfo {
    public:
        DWORD m_class;
        int m_level;
    };
    static const PriorityInfo s_pClass[] = {
        {IDLE_PRIORITY_CLASS, THREAD_PRIORITY_IDLE},
        {IDLE_PRIORITY_CLASS, THREAD_PRIORITY_LOWEST},
        {IDLE_PRIORITY_CLASS, THREAD_PRIORITY_BELOW_NORMAL},
        {IDLE_PRIORITY_CLASS, THREAD_PRIORITY_NORMAL},
        {IDLE_PRIORITY_CLASS, THREAD_PRIORITY_ABOVE_NORMAL},
        {IDLE_PRIORITY_CLASS, THREAD_PRIORITY_HIGHEST},
        {NORMAL_PRIORITY_CLASS, THREAD_PRIORITY_LOWEST},
        {NORMAL_PRIORITY_CLASS, THREAD_PRIORITY_BELOW_NORMAL},
        {NORMAL_PRIORITY_CLASS, THREAD_PRIORITY_NORMAL},
        {NORMAL_PRIORITY_CLASS, THREAD_PRIORITY_ABOVE_NORMAL},
        {NORMAL_PRIORITY_CLASS, THREAD_PRIORITY_HIGHEST},
        {HIGH_PRIORITY_CLASS, THREAD_PRIORITY_LOWEST},
        {HIGH_PRIORITY_CLASS, THREAD_PRIORITY_BELOW_NORMAL},
        {HIGH_PRIORITY_CLASS, THREAD_PRIORITY_NORMAL},
        {HIGH_PRIORITY_CLASS, THREAD_PRIORITY_ABOVE_NORMAL},
        {HIGH_PRIORITY_CLASS, THREAD_PRIORITY_HIGHEST},
        {REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_IDLE},
        {REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_LOWEST},
        {REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_BELOW_NORMAL},
        {REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_NORMAL},
        {REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_ABOVE_NORMAL},
        {REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_HIGHEST},
        {REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL}};
#if defined(_DEBUG)
    // don't use really high priorities when debugging
    static const size_t s_pMax = 13;
#else
    static const size_t s_pMax = sizeof (s_pClass) / sizeof (s_pClass[0]) - 1;
#endif
    static const size_t s_pBase = 8; // index of normal priority

    assert (thread != NULL);

    size_t index;
    if (n > 0 && s_pBase < (size_t) n) {
        // lowest priority
        index = 0;
    } else {
        index = (size_t) ((int) s_pBase - n);
        if (index > s_pMax) {
            // highest priority
            index = s_pMax;
        }
    }
    SetPriorityClass (GetCurrentProcess (), s_pClass[index].m_class);
    SetThreadPriority (thread->m_thread, s_pClass[index].m_level);
}

void
ArchMultithreadWindows::testCancelThread () {
    // find current thread
    lockMutex (m_threadMutex);
    ArchThreadImpl* thread = findNoRef (GetCurrentThreadId ());
    unlockMutex (m_threadMutex);

    // test cancel on thread
    testCancelThreadImpl (thread);
}

bool
ArchMultithreadWindows::wait (ArchThread target, double timeout) {
    assert (target != NULL);

    lockMutex (m_threadMutex);

    // find current thread
    ArchThreadImpl* self = findNoRef (GetCurrentThreadId ());

    // ignore wait if trying to wait on ourself
    if (target == self) {
        unlockMutex (m_threadMutex);
        return false;
    }

    // ref the target so it can't go away while we're watching it
    refThread (target);

    unlockMutex (m_threadMutex);

    // convert timeout
    DWORD t;
    if (timeout < 0.0) {
        t = INFINITE;
    } else {
        t = (DWORD) (1000.0 * timeout);
    }

    // wait for this thread to be cancelled or woken up or for the
    // target thread to terminate.
    HANDLE handles[2];
    handles[0]   = target->m_exit;
    handles[1]   = self->m_cancel;
    DWORD result = WaitForMultipleObjects (2, handles, FALSE, t);

    // cancel takes priority
    if (result != WAIT_OBJECT_0 + 1 &&
        WaitForSingleObject (handles[1], 0) == WAIT_OBJECT_0) {
        result = WAIT_OBJECT_0 + 1;
    }

    // release target
    closeThread (target);

    // handle result
    switch (result) {
        case WAIT_OBJECT_0 + 0:
            // target thread terminated
            return true;

        case WAIT_OBJECT_0 + 1:
            // this thread was cancelled.  does not return.
            testCancelThreadImpl (self);

        default:
            // timeout or error
            return false;
    }
}

bool
ArchMultithreadWindows::isSameThread (ArchThread thread1, ArchThread thread2) {
    return (thread1 == thread2);
}

bool
ArchMultithreadWindows::isExitedThread (ArchThread thread) {
    // poll exit event
    return (WaitForSingleObject (thread->m_exit, 0) == WAIT_OBJECT_0);
}

void*
ArchMultithreadWindows::getResultOfThread (ArchThread thread) {
    lockMutex (m_threadMutex);
    void* result = thread->m_result;
    unlockMutex (m_threadMutex);
    return result;
}

IArchMultithread::ThreadID
ArchMultithreadWindows::getIDOfThread (ArchThread thread) {
    return static_cast<ThreadID> (thread->m_id);
}

void
ArchMultithreadWindows::setSignalHandler (ESignal signal, SignalFunc func,
                                          void* userData) {
    lockMutex (m_threadMutex);
    m_signalFunc[signal]     = func;
    m_signalUserData[signal] = userData;
    unlockMutex (m_threadMutex);
}

void
ArchMultithreadWindows::raiseSignal (ESignal signal) {
    lockMutex (m_threadMutex);
    if (m_signalFunc[signal] != NULL) {
        m_signalFunc[signal](signal, m_signalUserData[signal]);
        ARCH->unblockPollSocket (m_mainThread);
    } else if (signal == kINTERRUPT || signal == kTERMINATE) {
        ARCH->cancelThread (m_mainThread);
    }
    unlockMutex (m_threadMutex);
}

ArchThreadImpl*
ArchMultithreadWindows::find (DWORD id) {
    ArchThreadImpl* impl = findNoRef (id);
    if (impl != NULL) {
        refThread (impl);
    }
    return impl;
}

ArchThreadImpl*
ArchMultithreadWindows::findNoRef (DWORD id) {
    ArchThreadImpl* impl = findNoRefOrCreate (id);
    if (impl == NULL) {
        // create thread for calling thread which isn't in our list and
        // add it to the list.  this won't normally happen but it can if
        // the system calls us under a new thread, like it does when we
        // run as a service.
        impl           = new ArchThreadImpl;
        impl->m_thread = NULL;
        impl->m_id     = GetCurrentThreadId ();
        insert (impl);
    }
    return impl;
}

ArchThreadImpl*
ArchMultithreadWindows::findNoRefOrCreate (DWORD id) {
    // linear search
    for (ThreadList::const_iterator index = m_threadList.begin ();
         index != m_threadList.end ();
         ++index) {
        if ((*index)->m_id == id) {
            return *index;
        }
    }
    return NULL;
}

void
ArchMultithreadWindows::insert (ArchThreadImpl* thread) {
    assert (thread != NULL);

    // thread shouldn't already be on the list
    assert (findNoRefOrCreate (thread->m_id) == NULL);

    // append to list
    m_threadList.push_back (thread);
}

void
ArchMultithreadWindows::erase (ArchThreadImpl* thread) {
    for (ThreadList::iterator index = m_threadList.begin ();
         index != m_threadList.end ();
         ++index) {
        if (*index == thread) {
            m_threadList.erase (index);
            break;
        }
    }
}

void
ArchMultithreadWindows::refThread (ArchThreadImpl* thread) {
    assert (thread != NULL);
    assert (findNoRefOrCreate (thread->m_id) != NULL);
    ++thread->m_refCount;
}

void
ArchMultithreadWindows::testCancelThreadImpl (ArchThreadImpl* thread) {
    assert (thread != NULL);

    // poll cancel event.  return if not set.
    const DWORD result = WaitForSingleObject (thread->m_cancel, 0);
    if (result != WAIT_OBJECT_0) {
        return;
    }

    // update cancel state
    lockMutex (m_threadMutex);
    bool cancel          = !thread->m_cancelling;
    thread->m_cancelling = true;
    ResetEvent (thread->m_cancel);
    unlockMutex (m_threadMutex);

    // unwind thread's stack if cancelling
    if (cancel) {
        throw XThreadCancel ();
    }
}

unsigned int __stdcall ArchMultithreadWindows::threadFunc (void* vrep) {
    // get the thread
    ArchThreadImpl* thread = static_cast<ArchThreadImpl*> (vrep);

    // run thread
    s_instance->doThreadFunc (thread);

    // terminate the thread
    return 0;
}

void
ArchMultithreadWindows::doThreadFunc (ArchThread thread) {
    // wait for parent to initialize this object
    lockMutex (m_threadMutex);
    unlockMutex (m_threadMutex);

    void* result = NULL;
    try {
        // go
        result = (*thread->m_func) (thread->m_userData);
    }

    catch (XThreadCancel&) {
        // client called cancel()
    } catch (...) {
        // note -- don't catch (...) to avoid masking bugs
        SetEvent (thread->m_exit);
        closeThread (thread);
        throw;
    }

    // thread has exited
    lockMutex (m_threadMutex);
    thread->m_result = result;
    unlockMutex (m_threadMutex);
    SetEvent (thread->m_exit);

    // done with thread
    closeThread (thread);
}

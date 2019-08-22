/*
 * barrier -- mouse and keyboard sharing utility
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

#include "arch/unix/ArchMultithreadPosix.h"

#include "arch/Arch.h"
#include "arch/XArch.h"

#include <signal.h>
#if TIME_WITH_SYS_TIME
#    include <sys/time.h>
#    include <time.h>
#else
#    if HAVE_SYS_TIME_H
#        include <sys/time.h>
#    else
#        include <time.h>
#    endif
#endif
#include <cerrno>

#define SIGWAKEUP SIGUSR1

#if !HAVE_PTHREAD_SIGNAL
    // boy, is this platform broken.  forget about pthread signal
    // handling and let signals through to every process.  barrier
    // will not terminate cleanly when it gets SIGTERM or SIGINT.
#    define pthread_sigmask sigprocmask
#    define pthread_kill(tid_, sig_) kill(0, (sig_))
#    define sigwait(set_, sig_)
#    undef HAVE_POSIX_SIGWAIT
#    define HAVE_POSIX_SIGWAIT 1
#endif

static
void
setSignalSet(sigset_t* sigset)
{
    sigemptyset(sigset);
    sigaddset(sigset, SIGHUP);
    sigaddset(sigset, SIGINT);
    sigaddset(sigset, SIGTERM);
    sigaddset(sigset, SIGUSR2);
}

//
// ArchThreadImpl
//

class ArchThreadImpl {
public:
    ArchThreadImpl();

public:
    int                    m_refCount;
    IArchMultithread::ThreadID        m_id;
    pthread_t            m_thread;
    IArchMultithread::ThreadFunc    m_func;
    void*                m_userData;
    bool                m_cancel;
    bool                m_cancelling;
    bool                m_exited;
    void*                m_result;
    void*                m_networkData;
};

ArchThreadImpl::ArchThreadImpl() :
    m_refCount(1),
    m_id(0),
    m_func(NULL),
    m_userData(NULL),
    m_cancel(false),
    m_cancelling(false),
    m_exited(false),
    m_result(NULL),
    m_networkData(NULL)
{
    // do nothing
}


//
// ArchMultithreadPosix
//

ArchMultithreadPosix*    ArchMultithreadPosix::s_instance = NULL;

ArchMultithreadPosix::ArchMultithreadPosix() :
    m_newThreadCalled(false),
    m_nextID(0)
{
    assert(s_instance == NULL);

    s_instance = this;

    // no signal handlers
    for (size_t i = 0; i < kNUM_SIGNALS; ++i) {
        m_signalFunc[i]     = NULL;
        m_signalUserData[i] = NULL;
    }

    // create thread for calling (main) thread and add it to our
    // list.  no need to lock the mutex since we're the only thread.
    m_mainThread           = new ArchThreadImpl;
    m_mainThread->m_thread = pthread_self();
    insert(m_mainThread);

    // install SIGWAKEUP handler.  this causes SIGWAKEUP to interrupt
    // system calls.  we use that when cancelling a thread to force it
    // to wake up immediately if it's blocked in a system call.  we
    // won't need this until another thread is created but it's fine
    // to install it now.
    struct sigaction act;
    sigemptyset(&act.sa_mask);
# if defined(SA_INTERRUPT)
    act.sa_flags   = SA_INTERRUPT;
# else
    act.sa_flags   = 0;
# endif
    act.sa_handler = &threadCancel;
    sigaction(SIGWAKEUP, &act, NULL);

    // set desired signal dispositions.  let SIGWAKEUP through but
    // ignore SIGPIPE (we'll handle EPIPE).
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGWAKEUP);
    pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);
}

ArchMultithreadPosix::~ArchMultithreadPosix()
{
    assert(s_instance != NULL);

    s_instance = NULL;
}

void
ArchMultithreadPosix::setNetworkDataForCurrentThread(void* data)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    ArchThreadImpl* thread = find(pthread_self());
    thread->m_networkData = data;
}

void*
ArchMultithreadPosix::getNetworkDataForThread(ArchThread thread)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    return thread->m_networkData;
}

ArchMultithreadPosix*
ArchMultithreadPosix::getInstance()
{
    return s_instance;
}

ArchCond
ArchMultithreadPosix::newCondVar()
{
    ArchCondImpl* cond = new ArchCondImpl;
    int status = pthread_cond_init(&cond->m_cond, NULL);
    (void)status;
    assert(status == 0);
    return cond;
}

void
ArchMultithreadPosix::closeCondVar(ArchCond cond)
{
    int status = pthread_cond_destroy(&cond->m_cond);
    (void)status;
    assert(status == 0);
    delete cond;
}

void
ArchMultithreadPosix::signalCondVar(ArchCond cond)
{
    int status = pthread_cond_signal(&cond->m_cond);
    (void)status;
    assert(status == 0);
}

void
ArchMultithreadPosix::broadcastCondVar(ArchCond cond)
{
    int status = pthread_cond_broadcast(&cond->m_cond);
    (void)status;
    assert(status == 0);
}

bool
ArchMultithreadPosix::waitCondVar(ArchCond cond,
                            ArchMutex mutex, double timeout)
{
    // we can't wait on a condition variable and also wake it up for
    // cancellation since we don't use posix cancellation.  so we
    // must wake up periodically to check for cancellation.  we
    // can't simply go back to waiting after the check since the
    // condition may have changed and we'll have lost the signal.
    // so we have to return to the caller.  since the caller will
    // always check for spurious wakeups the only drawback here is
    // performance:  we're waking up a lot more than desired.
    static const double maxCancellationLatency = 0.1;
    if (timeout < 0.0 || timeout > maxCancellationLatency) {
        timeout = maxCancellationLatency;
    }

    // see if we should cancel this thread
    testCancelThread();

    // get final time
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timespec finalTime;
    finalTime.tv_sec   = now.tv_sec;
    finalTime.tv_nsec  = now.tv_usec * 1000;
    long timeout_sec   = (long)timeout;
    long timeout_nsec  = (long)(1.0e+9 * (timeout - timeout_sec));
    finalTime.tv_sec  += timeout_sec;
    finalTime.tv_nsec += timeout_nsec;
    if (finalTime.tv_nsec >= 1000000000) {
        finalTime.tv_nsec -= 1000000000;
        finalTime.tv_sec  += 1;
    }

    // wait
    int status = pthread_cond_timedwait(&cond->m_cond,
                            &mutex->m_mutex, &finalTime);

    // check for cancel again
    testCancelThread();

    switch (status) {
    case 0:
        // success
        return true;

    case ETIMEDOUT:
        return false;

    default:
        assert(0 && "condition variable wait error");
        return false;
    }
}

ArchMutex
ArchMultithreadPosix::newMutex()
{
    pthread_mutexattr_t attr;
    int status = pthread_mutexattr_init(&attr);
    assert(status == 0);
    ArchMutexImpl* mutex = new ArchMutexImpl;
    status = pthread_mutex_init(&mutex->m_mutex, &attr);
    assert(status == 0);
    return mutex;
}

void
ArchMultithreadPosix::closeMutex(ArchMutex mutex)
{
    int status = pthread_mutex_destroy(&mutex->m_mutex);
    (void)status;
    assert(status == 0);
    delete mutex;
}

void
ArchMultithreadPosix::lockMutex(ArchMutex mutex)
{
    int status = pthread_mutex_lock(&mutex->m_mutex);

    switch (status) {
    case 0:
        // success
        return;

    case EDEADLK:
        assert(0 && "lock already owned");
        break;

    case EAGAIN:
        assert(0 && "too many recursive locks");
        break;

    default:
        assert(0 && "unexpected error");
        break;
    }
}

void
ArchMultithreadPosix::unlockMutex(ArchMutex mutex)
{
    int status = pthread_mutex_unlock(&mutex->m_mutex);

    switch (status) {
    case 0:
        // success
        return;

    case EPERM:
        assert(0 && "thread doesn't own a lock");
        break;

    default:
        assert(0 && "unexpected error");
        break;
    }
}

ArchThread
ArchMultithreadPosix::newThread(ThreadFunc func, void* data)
{
    assert(func != NULL);

    // initialize signal handler.  we do this here instead of the
    // constructor so we can avoid daemonizing (using fork())
    // when there are multiple threads.  clients can safely
    // use condition variables and mutexes before creating a
    // new thread and they can safely use the only thread
    // they have access to, the main thread, so they really
    // can't tell the difference.
    if (!m_newThreadCalled) {
        m_newThreadCalled = true;
#if HAVE_PTHREAD_SIGNAL
        startSignalHandler();
#endif
    }

    // note that the child thread will wait until we release this mutex
    std::lock_guard<std::mutex> lock(m_threadMutex);

    // create thread impl for new thread
    ArchThreadImpl* thread = new ArchThreadImpl;
    thread->m_func          = func;
    thread->m_userData      = data;

    // create the thread.  pthread_create() on RedHat 7.2 smp fails
    // if passed a NULL attr so use a default attr.
    pthread_attr_t attr;
    int status = pthread_attr_init(&attr);
    if (status == 0) {
        status = pthread_create(&thread->m_thread, &attr,
                            &ArchMultithreadPosix::threadFunc, thread);
        pthread_attr_destroy(&attr);
    }

    // check if thread was started
    if (status != 0) {
        // failed to start thread so clean up
        delete thread;
        thread = NULL;
    }
    else {
        // add thread to list
        insert(thread);

        // increment ref count to account for the thread itself
        refThread(thread);
    }

    return thread;
}

ArchThread
ArchMultithreadPosix::newCurrentThread()
{
    std::lock_guard<std::mutex> lock(m_threadMutex);

    ArchThreadImpl* thread = find(pthread_self());
    assert(thread != NULL);
    return thread;
}

void
ArchMultithreadPosix::closeThread(ArchThread thread)
{
    assert(thread != NULL);

    // decrement ref count and clean up thread if no more references
    if (--thread->m_refCount == 0) {
        // detach from thread (unless it's the main thread)
        if (thread->m_func != NULL) {
            pthread_detach(thread->m_thread);
        }

        // remove thread from list
        {
            std::lock_guard<std::mutex> lock(m_threadMutex);
            assert(findNoRef(thread->m_thread) == thread);
            erase(thread);
        }

        // done with thread
        delete thread;
    }
}

ArchThread
ArchMultithreadPosix::copyThread(ArchThread thread)
{
    refThread(thread);
    return thread;
}

void
ArchMultithreadPosix::cancelThread(ArchThread thread)
{
    assert(thread != NULL);

    // set cancel and wakeup flags if thread can be cancelled
    bool wakeup = false;

    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
        if (!thread->m_exited && !thread->m_cancelling) {
            thread->m_cancel = true;
            wakeup = true;
        }
    }

    // force thread to exit system calls if wakeup is true
    if (wakeup) {
        pthread_kill(thread->m_thread, SIGWAKEUP);
    }
}

void
ArchMultithreadPosix::setPriorityOfThread(ArchThread thread, int /*n*/)
{
    assert(thread != NULL);

    // FIXME
}

void
ArchMultithreadPosix::testCancelThread()
{
    // find current thread
    ArchThreadImpl* thread = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
        thread = findNoRef(pthread_self());
    }

    // test cancel on thread
    testCancelThreadImpl(thread);
}

bool
ArchMultithreadPosix::wait(ArchThread target, double timeout)
{
    assert(target != NULL);

    ArchThreadImpl* self = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_threadMutex);

        // find current thread
        self = findNoRef(pthread_self());

        // ignore wait if trying to wait on ourself
        if (target == self) {
            return false;
        }

        // ref the target so it can't go away while we're watching it
        refThread(target);
    }

    try {
        // do first test regardless of timeout
        testCancelThreadImpl(self);
        if (isExitedThread(target)) {
            closeThread(target);
            return true;
        }

        // wait and repeat test if there's a timeout
        if (timeout != 0.0) {
            const double start = ARCH->time();
            do {
                // wait a little
                ARCH->sleep(0.05);

                // repeat test
                testCancelThreadImpl(self);
                if (isExitedThread(target)) {
                    closeThread(target);
                    return true;
                }

                // repeat wait and test until timed out
            } while (timeout < 0.0 || (ARCH->time() - start) <= timeout);
        }

        closeThread(target);
        return false;
    }
    catch (...) {
        closeThread(target);
        throw;
    }
}

bool
ArchMultithreadPosix::isSameThread(ArchThread thread1, ArchThread thread2)
{
    return (thread1 == thread2);
}

bool
ArchMultithreadPosix::isExitedThread(ArchThread thread)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    return thread->m_exited;
}

void*
ArchMultithreadPosix::getResultOfThread(ArchThread thread)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    return thread->m_result;
}

IArchMultithread::ThreadID
ArchMultithreadPosix::getIDOfThread(ArchThread thread)
{
    return thread->m_id;
}

void
ArchMultithreadPosix::setSignalHandler(
                ESignal signal, SignalFunc func, void* userData)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    m_signalFunc[signal]     = func;
    m_signalUserData[signal] = userData;
}

void
ArchMultithreadPosix::raiseSignal(ESignal signal) 
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    if (m_signalFunc[signal] != NULL) {
        m_signalFunc[signal](signal, m_signalUserData[signal]);
        pthread_kill(m_mainThread->m_thread, SIGWAKEUP);
    }
    else if (signal == kINTERRUPT || signal == kTERMINATE) {
        ARCH->cancelThread(m_mainThread);
    }
}

void
ArchMultithreadPosix::startSignalHandler()
{
    // set signal mask.  the main thread blocks these signals and
    // the signal handler thread will listen for them.
    sigset_t sigset, oldsigset;
    setSignalSet(&sigset);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldsigset);

    // fire up the INT and TERM signal handler thread.  we could
    // instead arrange to catch and handle these signals but
    // we'd be unable to cancel the main thread since no pthread
    // calls are allowed in a signal handler.
    pthread_attr_t attr;
    int status = pthread_attr_init(&attr);
    if (status == 0) {
        status = pthread_create(&m_signalThread, &attr,
                            &ArchMultithreadPosix::threadSignalHandler,
                            NULL);
        pthread_attr_destroy(&attr);
    }
    if (status != 0) {
        // can't create thread to wait for signal so don't block
        // the signals.
        pthread_sigmask(SIG_UNBLOCK, &oldsigset, NULL);
    }
}

ArchThreadImpl*
ArchMultithreadPosix::find(pthread_t thread)
{
    ArchThreadImpl* impl = findNoRef(thread);
    if (impl != NULL) {
        refThread(impl);
    }
    return impl;
}

ArchThreadImpl*
ArchMultithreadPosix::findNoRef(pthread_t thread)
{
    // linear search
    for (ThreadList::const_iterator index  = m_threadList.begin();
                                     index != m_threadList.end(); ++index) {
        if ((*index)->m_thread == thread) {
            return *index;
        }
    }
    return NULL;
}

void
ArchMultithreadPosix::insert(ArchThreadImpl* thread)
{
    assert(thread != NULL);

    // thread shouldn't already be on the list
    assert(findNoRef(thread->m_thread) == NULL);

    // set thread id.  note that we don't worry about m_nextID
    // wrapping back to 0 and duplicating thread ID's since the
    // likelihood of barrier running that long is vanishingly
    // small.
    thread->m_id = ++m_nextID;

    // append to list
    m_threadList.push_back(thread);
}

void
ArchMultithreadPosix::erase(ArchThreadImpl* thread)
{
    for (ThreadList::iterator index  = m_threadList.begin();
                               index != m_threadList.end(); ++index) {
        if (*index == thread) {
            m_threadList.erase(index);
            break;
        }
    }
}

void
ArchMultithreadPosix::refThread(ArchThreadImpl* thread)
{
    assert(thread != NULL);
    assert(findNoRef(thread->m_thread) != NULL);
    ++thread->m_refCount;
}

void
ArchMultithreadPosix::testCancelThreadImpl(ArchThreadImpl* thread)
{
    assert(thread != NULL);

    std::lock_guard<std::mutex> lock(m_threadMutex);

    // update cancel state
    bool cancel = false;
    if (thread->m_cancel && !thread->m_cancelling) {
        thread->m_cancelling = true;
        thread->m_cancel     = false;
        cancel               = true;
    }

    // unwind thread's stack if cancelling
    if (cancel) {
        throw XThreadCancel();
    }
}

void*
ArchMultithreadPosix::threadFunc(void* vrep)
{
    // get the thread
    ArchThreadImpl* thread = static_cast<ArchThreadImpl*>(vrep);

    // setup pthreads
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    // run thread
    s_instance->doThreadFunc(thread);

    // terminate the thread
    return NULL;
}

void
ArchMultithreadPosix::doThreadFunc(ArchThread thread)
{
    // default priority is slightly below normal
    setPriorityOfThread(thread, 1);

    // wait for parent to initialize this object
    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
    }

    void* result = NULL;
    try {
        // go
        result = (*thread->m_func)(thread->m_userData);
    }

    catch (XThreadCancel&) {
        // client called cancel()
    }
    catch (...) {
        // note -- don't catch (...) to avoid masking bugs
        {
            std::lock_guard<std::mutex> lock(m_threadMutex);
            thread->m_exited = true;
        }
        closeThread(thread);
        throw;
    }

    // thread has exited
    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
        thread->m_result = result;
        thread->m_exited = true;
    }

    // done with thread
    closeThread(thread);
}

void
ArchMultithreadPosix::threadCancel(int)
{
    // do nothing
}

void*
ArchMultithreadPosix::threadSignalHandler(void*)
{
    // detach
    pthread_detach(pthread_self());

    // add signal to mask
    sigset_t sigset;
    setSignalSet(&sigset);

    // also wait on SIGABRT.  on linux (others?) this thread (process)
    // will persist after all the other threads evaporate due to an
    // assert unless we wait on SIGABRT.  that means our resources (like
    // the socket we're listening on) are not released and never will be
    // until the lingering thread is killed.  i don't know why sigwait()
    // should protect the thread from being killed.  note that sigwait()
    // doesn't actually return if we receive SIGABRT and, for some
    // reason, we don't have to block SIGABRT.
    sigaddset(&sigset, SIGABRT);

    // we exit the loop via thread cancellation in sigwait()
    for (;;) {
        // wait
#if HAVE_POSIX_SIGWAIT
        int signal = 0;
        sigwait(&sigset, &signal);
#else
        sigwait(&sigset);
#endif

        // if we get here then the signal was raised
        switch (signal) {
        case SIGINT:
            ARCH->raiseSignal(kINTERRUPT);
            break;

        case SIGTERM:
            ARCH->raiseSignal(kTERMINATE);
            break;

        case SIGHUP:
            ARCH->raiseSignal(kHANGUP);
            break;

        case SIGUSR2:
            ARCH->raiseSignal(kUSER);
            break;

        default:
            // ignore
            break;
        }
    }

    return NULL;
}

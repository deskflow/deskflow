/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CArchMultithreadPosix.h"
#include "CArch.h"
#include "XArch.h"
#include <signal.h>
#if TIME_WITH_SYS_TIME
#	include <sys/time.h>
#	include <time.h>
#else
#	if HAVE_SYS_TIME_H
#		include <sys/time.h>
#	else
#		include <time.h>
#	endif
#endif
#include <cerrno>

#define SIGWAKEUP SIGUSR1

#if !HAVE_PTHREAD_SIGNAL
	// boy, is this platform broken.  forget about pthread signal
	// handling and let signals through to every process.  synergy
	// will not terminate cleanly when it gets SIGTERM or SIGINT.
#	define pthread_sigmask sigprocmask
#	define pthread_kill(tid_, sig_) kill(0, (sig_))
#	define sigwait(set_, sig_)
#	undef HAVE_POSIX_SIGWAIT
#	define HAVE_POSIX_SIGWAIT 1
#endif

//
// CArchThreadImpl
//

class CArchThreadImpl {
public:
	CArchThreadImpl();

public:
	int					m_refCount;
	IArchMultithread::ThreadID		m_id;
	pthread_t			m_thread;
	IArchMultithread::ThreadFunc	m_func;
	void*				m_userData;
	bool				m_cancel;
	bool				m_cancelling;
	bool				m_exited;
	void*				m_result;
};

CArchThreadImpl::CArchThreadImpl() :
	m_refCount(1),
	m_id(0),
	m_func(NULL),
	m_userData(NULL),
	m_cancel(false),
	m_cancelling(false),
	m_exited(false),
	m_result(NULL)
{
	// do nothing
}


//
// CArchMultithreadPosix
//

CArchMultithreadPosix*	CArchMultithreadPosix::s_instance = NULL;

CArchMultithreadPosix::CArchMultithreadPosix() :
	m_newThreadCalled(false),
	m_nextID(0)
{
	assert(s_instance == NULL);

	s_instance = this;

	// create mutex for thread list
	m_threadMutex = newMutex();

	// create thread for calling (main) thread and add it to our
	// list.  no need to lock the mutex since we're the only thread.
	m_mainThread           = new CArchThreadImpl;
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

CArchMultithreadPosix::~CArchMultithreadPosix()
{
	assert(s_instance != NULL);

	closeMutex(m_threadMutex);
	s_instance = NULL;
}

CArchCond
CArchMultithreadPosix::newCondVar()
{
	CArchCondImpl* cond = new CArchCondImpl;
	int status = pthread_cond_init(&cond->m_cond, NULL);
	assert(status == 0);
	return cond;
}

void
CArchMultithreadPosix::closeCondVar(CArchCond cond)
{
	int status = pthread_cond_destroy(&cond->m_cond);
	assert(status == 0);
	delete cond;
}

void
CArchMultithreadPosix::signalCondVar(CArchCond cond)
{
	int status = pthread_cond_signal(&cond->m_cond);
	assert(status == 0);
}

void
CArchMultithreadPosix::broadcastCondVar(CArchCond cond)
{
	int status = pthread_cond_broadcast(&cond->m_cond);
	assert(status == 0);
}

bool
CArchMultithreadPosix::waitCondVar(CArchCond cond,
							CArchMutex mutex, double timeout)
{
	// get final time
	struct timeval now;
	gettimeofday(&now, NULL);
	struct timespec finalTime;
	finalTime.tv_sec  = now.tv_sec;
	finalTime.tv_nsec = now.tv_usec * 1000;
	if (timeout >= 0.0) {
		const long timeout_sec  = (long)timeout;
		const long timeout_nsec = (long)(1.0e+9 * (timeout - timeout_sec));
		finalTime.tv_sec  += timeout_sec;
		finalTime.tv_nsec += timeout_nsec;
		if (finalTime.tv_nsec >= 1000000000) {
			finalTime.tv_nsec -= 1000000000;
			finalTime.tv_sec  += 1;
		}
	}

	// repeat until we reach the final time
	int status;
	for (;;) {
		// get current time
		gettimeofday(&now, NULL);
		struct timespec endTime;
		endTime.tv_sec  = now.tv_sec;
		endTime.tv_nsec = now.tv_usec * 1000;

		// done if past final timeout
		if (timeout >= 0.0) {
			if (endTime.tv_sec > finalTime.tv_sec ||
				(endTime.tv_sec  == finalTime.tv_sec &&
				 endTime.tv_nsec >= finalTime.tv_nsec)) {
				status = ETIMEDOUT;
				break;
			}
		}

		// compute the next timeout
		endTime.tv_nsec += 50000000;
		if (endTime.tv_nsec >= 1000000000) {
			endTime.tv_nsec -= 1000000000;
			endTime.tv_sec  += 1;
		}

		// don't wait past final timeout
		if (timeout >= 0.0) {
			if (endTime.tv_sec > finalTime.tv_sec ||
				(endTime.tv_sec  == finalTime.tv_sec &&
				 endTime.tv_nsec >= finalTime.tv_nsec)) {
				endTime = finalTime;
			}
		}

		// see if we should cancel this thread
		testCancelThread();

		// wait
		status = pthread_cond_timedwait(&cond->m_cond,
							&mutex->m_mutex, &endTime);

		// check for cancel again
		testCancelThread();

		// check wait status
		if (status != ETIMEDOUT && status != EINTR) {
			break;
		}
	}

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

CArchMutex
CArchMultithreadPosix::newMutex()
{
	CArchMutexImpl* mutex = new CArchMutexImpl;
	int status = pthread_mutex_init(&mutex->m_mutex, NULL);
	assert(status == 0);
/*
	status = pthread_mutexattr_settype(&mutex->m_mutex,
							PTHREAD_MUTEX_RECURSIVE);
	assert(status == 0);
*/
	return mutex;
}

void
CArchMultithreadPosix::closeMutex(CArchMutex mutex)
{
	int status = pthread_mutex_destroy(&mutex->m_mutex);
	assert(status == 0);
	delete mutex;
}

void
CArchMultithreadPosix::lockMutex(CArchMutex mutex)
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
CArchMultithreadPosix::unlockMutex(CArchMutex mutex)
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

CArchThread
CArchMultithreadPosix::newThread(ThreadFunc func, void* data)
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

	lockMutex(m_threadMutex);

	// create thread impl for new thread
	CArchThreadImpl* thread = new CArchThreadImpl;
	thread->m_func          = func;
	thread->m_userData      = data;

	// mask some signals in all threads except the main thread
	sigset_t sigset, oldsigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &sigset, &oldsigset);

	// create the thread.  pthread_create() on RedHat 7.2 smp fails
	// if passed a NULL attr so use a default attr.
	pthread_attr_t attr;
	int status = pthread_attr_init(&attr);
	if (status == 0) {
		status = pthread_create(&thread->m_thread, &attr,
							&CArchMultithreadPosix::threadFunc, thread);
		pthread_attr_destroy(&attr);
	}

	// restore signals
	pthread_sigmask(SIG_SETMASK, &oldsigset, NULL);

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

	// note that the child thread will wait until we release this mutex
	unlockMutex(m_threadMutex);

	return thread;
}

CArchThread
CArchMultithreadPosix::newCurrentThread()
{
	lockMutex(m_threadMutex);
	CArchThreadImpl* thread = find(pthread_self());
	unlockMutex(m_threadMutex);
	assert(thread != NULL);
	return thread;
}

void
CArchMultithreadPosix::closeThread(CArchThread thread)
{
	assert(thread != NULL);

	// decrement ref count and clean up thread if no more references
	if (--thread->m_refCount == 0) {
		// detach from thread (unless it's the main thread)
		if (thread->m_func != NULL) {
			pthread_detach(thread->m_thread);
		}

		// remove thread from list
		lockMutex(m_threadMutex);
		assert(findNoRef(thread->m_thread) == thread);
		erase(thread);
		unlockMutex(m_threadMutex);

		// done with thread
		delete thread;
	}
}

CArchThread
CArchMultithreadPosix::copyThread(CArchThread thread)
{
	refThread(thread);
	return thread;
}

void
CArchMultithreadPosix::cancelThread(CArchThread thread)
{
	assert(thread != NULL);

	// set cancel and wakeup flags if thread can be cancelled
	bool wakeup = false;
	lockMutex(m_threadMutex);
	if (!thread->m_exited && !thread->m_cancelling) {
		thread->m_cancel = true;
		wakeup = true;
	}
	unlockMutex(m_threadMutex);

	// force thread to exit system calls if wakeup is true
	if (wakeup) {
		pthread_kill(thread->m_thread, SIGWAKEUP);
	}
}

void
CArchMultithreadPosix::setPriorityOfThread(CArchThread thread, int /*n*/)
{
	assert(thread != NULL);

	// FIXME
}

void
CArchMultithreadPosix::testCancelThread()
{
	// find current thread
	lockMutex(m_threadMutex);
	CArchThreadImpl* thread = findNoRef(pthread_self());
	unlockMutex(m_threadMutex);

	// test cancel on thread
	testCancelThreadImpl(thread);
}

bool
CArchMultithreadPosix::wait(CArchThread target, double timeout)
{
	assert(target != NULL);

	lockMutex(m_threadMutex);

	// find current thread
	CArchThreadImpl* self = findNoRef(pthread_self());

	// ignore wait if trying to wait on ourself
	if (target == self) {
		unlockMutex(m_threadMutex);
		return false;
	}

	// ref the target so it can't go away while we're watching it
	refThread(target);

	unlockMutex(m_threadMutex);

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

IArchMultithread::EWaitResult
CArchMultithreadPosix::waitForEvent(CArchThread, double /*timeout*/)
{
	// not implemented
	return kTimeout;
}

bool
CArchMultithreadPosix::isSameThread(CArchThread thread1, CArchThread thread2)
{
	return (thread1 == thread2);
}

bool
CArchMultithreadPosix::isExitedThread(CArchThread thread)
{
	lockMutex(m_threadMutex);
	bool exited = thread->m_exited;
	unlockMutex(m_threadMutex);
	return exited;
}

void*
CArchMultithreadPosix::getResultOfThread(CArchThread thread)
{
	lockMutex(m_threadMutex);
	void* result = thread->m_result;
	unlockMutex(m_threadMutex);
	return result;
}

IArchMultithread::ThreadID
CArchMultithreadPosix::getIDOfThread(CArchThread thread)
{
	return thread->m_id;
}

void
CArchMultithreadPosix::startSignalHandler()
{
	// set signal mask.  the main thread blocks these signals and
	// the signal handler thread will listen for them.
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	// fire up the INT and TERM signal handler thread.  we could
	// instead arrange to catch and handle these signals but
	// we'd be unable to cancel the main thread since no pthread
	// calls are allowed in a signal handler.
	pthread_attr_t attr;
	int status = pthread_attr_init(&attr);
	if (status == 0) {
		status = pthread_create(&m_signalThread, &attr,
							&CArchMultithreadPosix::threadSignalHandler,
							m_mainThread);
		pthread_attr_destroy(&attr);
	}
	if (status != 0) {
		// can't create thread to wait for signal so don't block
		// the signals.
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGINT);
		sigaddset(&sigset, SIGTERM);
		pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
	}
}

CArchThreadImpl*
CArchMultithreadPosix::find(pthread_t thread)
{
	CArchThreadImpl* impl = findNoRef(thread);
	if (impl != NULL) {
		refThread(impl);
	}
	return impl;
}

CArchThreadImpl*
CArchMultithreadPosix::findNoRef(pthread_t thread)
{
	// linear search
	for (CThreadList::const_iterator index  = m_threadList.begin();
									 index != m_threadList.end(); ++index) {
		if ((*index)->m_thread == thread) {
			return *index;
		}
	}
	return NULL;
}

void
CArchMultithreadPosix::insert(CArchThreadImpl* thread)
{
	assert(thread != NULL);

	// thread shouldn't already be on the list
	assert(findNoRef(thread->m_thread) == NULL);

	// set thread id.  note that we don't worry about m_nextID
	// wrapping back to 0 and duplicating thread ID's since the
	// likelihood of synergy running that long is vanishingly
	// small.
	thread->m_id = ++m_nextID;

	// append to list
	m_threadList.push_back(thread);
}

void
CArchMultithreadPosix::erase(CArchThreadImpl* thread)
{
	for (CThreadList::iterator index  = m_threadList.begin();
							   index != m_threadList.end(); ++index) {
		if (*index == thread) {
			m_threadList.erase(index);
			break;
		}
	}
}

void
CArchMultithreadPosix::refThread(CArchThreadImpl* thread)
{
	assert(thread != NULL);
	assert(findNoRef(thread->m_thread) != NULL);
	++thread->m_refCount;
}

void
CArchMultithreadPosix::testCancelThreadImpl(CArchThreadImpl* thread)
{
	assert(thread != NULL);

	// update cancel state
	lockMutex(m_threadMutex);
	bool cancel = false;
	if (thread->m_cancel && !thread->m_cancelling) {
		thread->m_cancelling = true;
		thread->m_cancel     = false;
		cancel               = true;
	}
	unlockMutex(m_threadMutex);

	// unwind thread's stack if cancelling
	if (cancel) {
		throw XThreadCancel();
	}
}

void*
CArchMultithreadPosix::threadFunc(void* vrep)
{
	// get the thread
	CArchThreadImpl* thread = reinterpret_cast<CArchThreadImpl*>(vrep);

	// setup pthreads
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	// run thread
	s_instance->doThreadFunc(thread);

	// terminate the thread
	return NULL;
}

void
CArchMultithreadPosix::doThreadFunc(CArchThread thread)
{
	// default priority is slightly below normal
	setPriorityOfThread(thread, 1);

	// wait for parent to initialize this object
	lockMutex(m_threadMutex);
	unlockMutex(m_threadMutex);

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
		lockMutex(m_threadMutex);
		thread->m_exited = true;
		unlockMutex(m_threadMutex);
		closeThread(thread);
		throw;
	}

	// thread has exited
	lockMutex(m_threadMutex);
	thread->m_result = result;
	thread->m_exited = true;
	unlockMutex(m_threadMutex);

	// done with thread
	closeThread(thread);
}

void
CArchMultithreadPosix::threadCancel(int)
{
	// do nothing
}

void*
CArchMultithreadPosix::threadSignalHandler(void* vrep)
{
	CArchThreadImpl* mainThread = reinterpret_cast<CArchThreadImpl*>(vrep);

	// detach
	pthread_detach(pthread_self());

	// add signal to mask
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);

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
		int signal;
		sigwait(&sigset, &signal);
#else
		sigwait(&sigset);
#endif

		// if we get here then the signal was raised.  cancel the main
		// thread so it can shut down cleanly.
		ARCH->cancelThread(mainThread);
	}
}

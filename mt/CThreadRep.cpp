#include "CThreadRep.h"
#include "CThread.h"
#include "CMutex.h"
#include "CLock.h"
#include "XThread.h"
#include "CLog.h"
#include "IJob.h"
#include <assert.h>

#if defined(CONFIG_PTHREADS)
#include <signal.h>
#define SIGWAKEUP SIGUSR1
#endif

#if defined(CONFIG_PLATFORM_WIN32)
# if !defined(_MT)
#  error multithreading compile option is required
# endif
#include <process.h>
#endif

// FIXME -- temporary exception type
class XThreadUnavailable { };

#if defined(CONFIG_PLATFORM_UNIX) && !defined(NDEBUG)
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
static void threadDebug(int)
{
	if (fork() == 0) abort();
	else { wait(0); exit(1); }
}
#endif

//
// CThreadRep
//

CMutex*					CThreadRep::s_mutex = NULL;
CThreadRep*				CThreadRep::s_head = NULL;
#if defined(CONFIG_PTHREADS)
pthread_t				CThreadRep::s_signalThread;
#endif

CThreadRep::CThreadRep() : m_prev(NULL),
								m_next(NULL),
								m_refCount(1),
								m_job(NULL),
								m_userData(NULL)
{
	// note -- s_mutex must be locked on entry
	assert(s_mutex != NULL);

	// initialize stuff
	init();
#if defined(CONFIG_PTHREADS)
	// get main thread id
	m_thread = pthread_self();
#elif defined(CONFIG_PLATFORM_WIN32)
	// get main thread id
	m_thread = NULL;
	m_id     = GetCurrentThreadId();
#endif

	// insert ourself into linked list
	if (s_head != NULL) {
		s_head->m_prev = this;
		m_next         = s_head;
	}
	s_head = this;
}

CThreadRep::CThreadRep(IJob* job, void* userData) :
								m_prev(NULL),
								m_next(NULL),
								m_refCount(2),	// 1 for us, 1 for thread
								m_job(job),
								m_userData(userData)
{
	assert(m_job != NULL);
	assert(s_mutex != NULL);

	// create a thread rep for the main thread if the current thread
	// is unknown.  note that this might cause multiple "main" threads
	// if threads are created external to this library.
	getCurrentThreadRep()->unref();

	// initialize
	init();

	// hold mutex while we create the thread
	CLock lock(s_mutex);

	// start the thread.  throw if it doesn't start.
#if defined(CONFIG_PTHREADS)
	// mask some signals in all threads except the main thread
	sigset_t sigset, oldsigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &sigset, &oldsigset);
	int status = pthread_create(&m_thread, NULL, threadFunc, (void*)this);
	pthread_sigmask(SIG_SETMASK, &oldsigset, NULL);
	if (status != 0)
		throw XThreadUnavailable();
#elif defined(CONFIG_PLATFORM_WIN32)
	unsigned int id;
	m_thread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0,
								threadFunc, (void*)this, 0, &id));
	m_id     = static_cast<DWORD>(id);
	if (m_thread == 0)
		throw XThreadUnavailable();
#endif

	// insert ourself into linked list
	if (s_head != NULL) {
		s_head->m_prev = this;
		m_next         = s_head;
	}
	s_head = this;

	// returning releases the locks, allowing the child thread to run
}

CThreadRep::~CThreadRep()
{
	// note -- s_mutex must be locked on entry

	// remove ourself from linked list
	if (m_prev != NULL) {
		m_prev->m_next = m_next;
	}
	if (m_next != NULL) {
		m_next->m_prev = m_prev;
	}
	if (s_head == this) {
		s_head = m_next;
	}

	// clean up
	fini();
}

void					CThreadRep::initThreads()
{
	if (s_mutex == NULL) {
		s_mutex = new CMutex;

#if defined(CONFIG_PTHREADS)
		// install SIGWAKEUP handler
		struct sigaction act;
		sigemptyset(&act.sa_mask);
# if defined(SA_INTERRUPT)
		act.sa_flags   = SA_INTERRUPT;
# else
		act.sa_flags   = 0;
# endif
		act.sa_handler = &threadCancel;
		sigaction(SIGWAKEUP, &act, NULL);
# ifndef NDEBUG
		act.sa_handler = &threadDebug;
		sigaction(SIGSEGV, &act, NULL);
# endif

		// set signal mask
		sigset_t sigset;
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGWAKEUP);
# ifndef NDEBUG
		sigaddset(&sigset, SIGSEGV);
# endif
		pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGPIPE);
		sigaddset(&sigset, SIGINT);
		sigaddset(&sigset, SIGTERM);
		pthread_sigmask(SIG_BLOCK, &sigset, NULL);

		// fire up the INT and TERM signal handler thread
		int status = pthread_create(&s_signalThread, NULL,
								&CThreadRep::threadSignalHandler,
								getCurrentThreadRep());
		if (status != 0) {
			// can't create thread to wait for signal so don't block
			// the signals.
			sigemptyset(&sigset);
			sigaddset(&sigset, SIGINT);
			sigaddset(&sigset, SIGTERM);
			pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
		}
#endif
	}
}

void					CThreadRep::ref()
{
	CLock lock(s_mutex);
	++m_refCount;
}

void					CThreadRep::unref()
{
	CLock lock(s_mutex);
	if (--m_refCount == 0) {
		delete this;
	}
}

bool					CThreadRep::enableCancel(bool enable)
{
	CLock lock(s_mutex);
	const bool old = m_cancellable;
	m_cancellable = enable;
	return old;
}

bool					CThreadRep::isCancellable() const
{
	CLock lock(s_mutex);
	return (m_cancellable && !m_cancelling);
}

void*					CThreadRep::getResult() const
{
	// no lock necessary since thread isn't running
	return m_result;
}

void*					CThreadRep::getUserData() const
{
	// no lock necessary because the value never changes
	return m_userData;
}

CThreadRep*				CThreadRep::getCurrentThreadRep()
{
	assert(s_mutex != NULL);

#if defined(CONFIG_PTHREADS)
	const pthread_t thread = pthread_self();	
#elif defined(CONFIG_PLATFORM_WIN32)
	const DWORD id = GetCurrentThreadId();
#endif

	// lock list while we search
	CLock lock(s_mutex);

	// search
	CThreadRep* scan = s_head;
	while (scan != NULL) {
#if defined(CONFIG_PTHREADS)
		if (scan->m_thread == thread) {
			break;
		}
#elif defined(CONFIG_PLATFORM_WIN32)
		if (scan->m_id == id) {
			break;
		}
#endif
		scan = scan->m_next;
	}

	// create and use main thread rep if thread not found
	if (scan == NULL) {
		scan = new CThreadRep();
	}

	// ref for caller
	++scan->m_refCount;

	return scan;
}

void					CThreadRep::doThreadFunc()
{
	// default priority is slightly below normal
	setPriority(1);

	// wait for parent to initialize this object
	{ CLock lock(s_mutex); }

	void* result = NULL;
	try {
		// go
		m_job->run();
	}

	catch (XThreadCancel&) {
		// client called cancel()
		log((CLOG_DEBUG1 "caught cancel on thread %p", this));
	}

	catch (XThreadExit& e) {
		// client called exit()
		result = e.m_result;
		log((CLOG_DEBUG1 "caught exit on thread %p", this));
	}
	catch (...) {
		log((CLOG_DEBUG1 "exception on thread %p", this));
		// note -- don't catch (...) to avoid masking bugs
		delete m_job;
		throw;
	}

	// done with job
	delete m_job;

	// store exit result (no lock necessary because the result will
	// not be accessed until m_exit is set)
	m_result = result;
}

#if defined(CONFIG_PTHREADS)

#include "CStopwatch.h"
#include <time.h>

void					CThreadRep::init()
{
	m_result      = NULL;
	m_cancellable = true;
	m_cancelling  = false;
	m_cancel      = false;
	m_exit        = false;
}

void					CThreadRep::fini()
{
	// main thread has NULL job
	if (m_job != NULL) {
		pthread_detach(m_thread);
	}
}

void					CThreadRep::sleep(double timeout)
{
	if (timeout < 0.0)
		return;
	struct timespec t;
	t.tv_sec  = (long)timeout;
	t.tv_nsec = (long)(1000000000.0 * (timeout - (double)t.tv_sec));
	while (nanosleep(&t, &t) < 0)
		testCancel();
}

void					CThreadRep::cancel()
{
	CLock lock(s_mutex);
	if (m_cancellable && !m_cancelling) {
		m_cancel = true;
	}
	else {
		return;
	}

	// break out of system calls
	log((CLOG_DEBUG1 "cancel thread %p", this));
	pthread_kill(m_thread, SIGWAKEUP);
}

void					CThreadRep::testCancel()
{
	{
		CLock lock(s_mutex);

		// done if not cancelled, not cancellable, or already cancelling
		if (!m_cancel || !m_cancellable || m_cancelling)
			return;

		// update state for cancel
		m_cancel     = false;
		m_cancelling = true;
	}

	// start cancel
	log((CLOG_DEBUG1 "throw cancel on thread %p", this));
	throw XThreadCancel();
}

bool					CThreadRep::wait(CThreadRep* target, double timeout)
{
	if (target == this)
		return false;

	testCancel();
	if (target->isExited())
		return true;

	if (timeout != 0.0) {
		CStopwatch timer;
		do {
			sleep(0.05);
			testCancel();
			if (target->isExited())
				return true;
		} while (timeout < 0.0 || timer.getTime() <= timeout);
	}

	return false;
}

void					CThreadRep::setPriority(int)
{
	// FIXME
}

bool					CThreadRep::isExited() const
{
	CLock lock(s_mutex);
	return m_exit;
}

void*					CThreadRep::threadFunc(void* arg)
{
	CThreadRep* rep = (CThreadRep*)arg;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	// run thread
	rep->doThreadFunc();

	{
		// mark as terminated
		CLock lock(s_mutex);
		rep->m_exit = true;
	}

	// unref thread
	rep->unref();

	// terminate the thread
	return NULL;
}

void					CThreadRep::threadCancel(int)
{
	// do nothing
}

void*					CThreadRep::threadSignalHandler(void* vrep)
{
	CThreadRep* mainThreadRep = reinterpret_cast<CThreadRep*>(vrep);

	// add signal to mask
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);

	// we exit the loop via thread cancellation in sigwait()
	for (;;) {
		// wait
		int signal;
		sigwait(&sigset, &signal);

		// if we get here then the signal was raised.  cancel the thread.
		mainThreadRep->cancel();
	}
}

#elif defined(CONFIG_PLATFORM_WIN32)

void					CThreadRep::init()
{
	m_result      = NULL;
	m_cancellable = true;
	m_cancelling  = false;
	m_exit        = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_cancel      = CreateEvent(NULL, TRUE, FALSE, NULL);
}

void					CThreadRep::fini()
{
	// destroy the events
	CloseHandle(m_cancel);
	CloseHandle(m_exit);

	// close the handle (main thread has a NULL handle)
	if (m_thread != NULL) {
		CloseHandle(m_thread);
	}
}

void					CThreadRep::sleep(double timeout)
{
	if (isCancellable())
		WaitForSingleObject(m_cancel, (DWORD)(1000.0 * timeout));
	else
		Sleep((DWORD)(1000.0 * timeout));
}

void					CThreadRep::cancel()
{
	log((CLOG_DEBUG1 "cancel thread %p", this));
	SetEvent(m_cancel);
}

void					CThreadRep::testCancel()
{
	// poll cancel event.  return if not set.
	const DWORD result = WaitForSingleObject(getCancelEvent(), 0);
	if (result != WAIT_OBJECT_0)
		return;

	{
		// ignore if disabled or already cancelling
		CLock lock(s_mutex);
		if (!m_cancellable || m_cancelling)
			return;

		// update state for cancel
		m_cancelling = true;
		ResetEvent(m_cancel);
	}

	// start cancel
	log((CLOG_DEBUG1 "throw cancel on thread %p", this));
	throw XThreadCancel();
}

bool					CThreadRep::wait(CThreadRep* target, double timeout)
{
	// get the current thread.  if it's the same as the target thread
	// then the thread is waiting on itself.
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	if (target == this)
		return false;

	// is cancellation enabled?
	const DWORD n = (isCancellable() ? 2 : 1);

	// convert timeout
	DWORD t;
	if (timeout < 0.0)
		t = INFINITE;
	else
		t = (DWORD)(1000.0 * timeout);

	// wait for this thread to be cancelled or for the target thread to
	// terminate.
	HANDLE handles[2];
	handles[0] = target->getExitEvent();
	handles[1] = m_cancel;
	DWORD result = WaitForMultipleObjects(n, handles, FALSE, t);

	// cancel takes priority
	if (n == 2 && result != WAIT_OBJECT_0 + 1 &&
					WaitForSingleObject(handles[1], 0) == WAIT_OBJECT_0)
		result = WAIT_OBJECT_0 + 1;

	// handle result
	switch (result) {
	case WAIT_OBJECT_0 + 0:
		// target thread terminated
		return true;

	case WAIT_OBJECT_0 + 1:
		// this thread was cancelled.  does not return.
		testCancel();

	default:
		// error
		return false;
	}
}

void					CThreadRep::setPriority(int n)
{
	DWORD pClass = NORMAL_PRIORITY_CLASS;
	if (n < 0) {
		switch (-n) {
		case 1:  n = THREAD_PRIORITY_ABOVE_NORMAL; break;
		case 2:  n = THREAD_PRIORITY_HIGHEST; break;
		default:
			pClass = HIGH_PRIORITY_CLASS;
			switch (-n - 3) {
			case 0:  n = THREAD_PRIORITY_LOWEST; break;
			case 1:  n = THREAD_PRIORITY_BELOW_NORMAL; break;
			case 2:  n = THREAD_PRIORITY_NORMAL; break;
			case 3:  n = THREAD_PRIORITY_ABOVE_NORMAL; break;
			default: n = THREAD_PRIORITY_HIGHEST; break;
			}
			break;
		}
	}
	else {
		switch (n) {
		case 0:  n = THREAD_PRIORITY_NORMAL; break;
		case 1:  n = THREAD_PRIORITY_BELOW_NORMAL; break;
		case 2:  n = THREAD_PRIORITY_LOWEST; break;
		default: n = THREAD_PRIORITY_IDLE; break;
		}
	}
	SetPriorityClass(m_thread, pClass);
	SetThreadPriority(m_thread, n);
}

HANDLE					CThreadRep::getExitEvent() const
{
	// no lock necessary because the value never changes
	return m_exit;
}

HANDLE					CThreadRep::getCancelEvent() const
{
	// no lock necessary because the value never changes
	return m_cancel;
}

unsigned int __stdcall	CThreadRep::threadFunc(void* arg)
{
	CThreadRep* rep = (CThreadRep*)arg;

	// initialize OLE
	const HRESULT hr = OleInitialize(NULL);

	// run thread
	rep->doThreadFunc();

	// close OLE
	if (!FAILED(hr)) {
		OleUninitialize();
	}

	// signal termination
	SetEvent(rep->m_exit);

	// unref thread
	rep->unref();

	// terminate the thread
	return 0;
}

#endif

#include "CCondVar.h"
#include "CStopwatch.h"

//
// CCondVarBase
//

CCondVarBase::CCondVarBase(CMutex* mutex) : 
	m_mutex(mutex)
#if defined(CONFIG_PLATFORM_WIN32)
	, m_waitCountMutex()
#endif
{
	assert(m_mutex != NULL);
	init();
}

CCondVarBase::~CCondVarBase()
{
	fini();
}

void
CCondVarBase::lock() const
{
	m_mutex->lock();
}

void
CCondVarBase::unlock() const
{
	m_mutex->unlock();
}

bool
CCondVarBase::wait(double timeout) const
{
	CStopwatch timer(true);
	return wait(timer, timeout);
}

CMutex*
CCondVarBase::getMutex() const
{
	return m_mutex;
}

#if defined(CONFIG_PTHREADS)

#include "CThread.h"
#include <pthread.h>
#include <sys/time.h>
#include <cerrno>

void
CCondVarBase::init()
{
	pthread_cond_t* cond = new pthread_cond_t;
	int status = pthread_cond_init(cond, NULL);
	assert(status == 0);
	m_cond = reinterpret_cast<pthread_cond_t*>(cond);
}

void
CCondVarBase::fini()
{
	pthread_cond_t* cond = reinterpret_cast<pthread_cond_t*>(m_cond);
	int status = pthread_cond_destroy(cond);
	assert(status == 0);
	delete cond;
}

void
CCondVarBase::signal()
{
	pthread_cond_t* cond = reinterpret_cast<pthread_cond_t*>(m_cond);
	int status = pthread_cond_signal(cond);
	assert(status == 0);
}

void
CCondVarBase::broadcast()
{
	pthread_cond_t* cond = reinterpret_cast<pthread_cond_t*>(m_cond);
	int status = pthread_cond_broadcast(cond);
	assert(status == 0);
}

bool
CCondVarBase::wait(
	CStopwatch& timer,
	double timeout) const
{
	// check timeout against timer
	if (timeout >= 0.0) {
		timeout -= timer.getTime();
		if (timeout < 0.0)
			return false;
	}

	// get condition variable and mutex
	pthread_cond_t*  cond  = reinterpret_cast<pthread_cond_t*>(m_cond);
	pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_mutex->m_mutex);

	// get final time
	struct timeval now;
	gettimeofday(&now, NULL);
	struct timespec finalTime;
	finalTime.tv_sec  = now.tv_sec;
	finalTime.tv_nsec = now.tv_usec * 1000;
	if (timeout >= 0.0) {
		const long timeout_sec  = (long)timeout;
		const long timeout_nsec = (long)(1000000000.0 * (timeout - timeout_sec));
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
		// compute the next timeout
		gettimeofday(&now, NULL);
		struct timespec endTime;
		endTime.tv_sec  = now.tv_sec;
		endTime.tv_nsec = now.tv_usec * 1000 + 50000000;
		if (endTime.tv_nsec >= 1000000000) {
			endTime.tv_nsec -= 1000000000;
			endTime.tv_sec  += 1;
		}

		// see if we should cancel this thread
		CThread::testCancel();

		// done if past final timeout
		if (timeout >= 0.0) {
			if (endTime.tv_sec > finalTime.tv_sec ||
				(endTime.tv_sec  == finalTime.tv_sec &&
				 endTime.tv_nsec >= finalTime.tv_nsec)) {
				status = ETIMEDOUT;
				break;
			}
		}

		// wait
		status = pthread_cond_timedwait(cond, mutex, &endTime);

		// check for cancel again
		CThread::testCancel();

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

#endif // CONFIG_PTHREADS

#if defined(CONFIG_PLATFORM_WIN32)

#include "CLock.h"
#include "CThreadRep.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//
// note -- implementation taken from
//   http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
// titled "Strategies for Implementing POSIX Condition Variables
// on Win32."  it also provides an implementation that doesn't
// suffer from the incorrectness problem described in our
// corresponding header but it is slower, still unfair, and
// can cause busy waiting.
//

void
CCondVarBase::init()
{
	// prepare events
	HANDLE* events     = new HANDLE[2];
	events[kSignal]    = CreateEvent(NULL, FALSE, FALSE, NULL);
	events[kBroadcast] = CreateEvent(NULL, TRUE,  FALSE, NULL);

	// prepare members
	m_cond      = reinterpret_cast<void*>(events);
	m_waitCount = 0;
}

void
CCondVarBase::fini()
{
	HANDLE* events = reinterpret_cast<HANDLE*>(m_cond);
	CloseHandle(events[kSignal]);
	CloseHandle(events[kBroadcast]);
	delete[] events;
}

void
CCondVarBase::signal()
{
	// is anybody waiting?
	bool hasWaiter;
	{
		CLock lock(&m_waitCountMutex);
		hasWaiter = (m_waitCount > 0);
	}

	// wake one thread if anybody is waiting
	if (hasWaiter) {
		SetEvent(reinterpret_cast<HANDLE*>(m_cond)[kSignal]);
	}
}

void
CCondVarBase::broadcast()
{
	// is anybody waiting?
	bool hasWaiter;
	{
		CLock lock(&m_waitCountMutex);
		hasWaiter = (m_waitCount > 0);
	}

	// wake all threads if anybody is waiting
	if (hasWaiter) {
		SetEvent(reinterpret_cast<HANDLE*>(m_cond)[kBroadcast]);
	}
}

bool
CCondVarBase::wait(
	CStopwatch& timer,
	double timeout) const
{
	// check timeout against timer
	if (timeout >= 0.0) {
		timeout -= timer.getTime();
		if (timeout < 0.0) {
			return false;
		}
	}

	// prepare to wait
	CThreadPtr currentRep = CThreadRep::getCurrentThreadRep();
	const DWORD winTimeout = (timeout < 0.0) ? INFINITE :
								static_cast<DWORD>(1000.0 * timeout);
	HANDLE* events = reinterpret_cast<HANDLE*>(m_cond);
	HANDLE handles[3];
	handles[0] = events[kSignal];
	handles[1] = events[kBroadcast];
	handles[2] = currentRep->getCancelEvent();
	const DWORD n = currentRep->isCancellable() ? 3 : 2;

	// update waiter count
	{
		CLock lock(&m_waitCountMutex);
		++m_waitCount;
	}

	// release mutex.  this should be atomic with the wait so that it's
	// impossible for another thread to signal us between the unlock and
	// the wait, which would lead to a lost signal on broadcasts.
	// however, we're using a manual reset event for broadcasts which
	// stays set until we reset it, so we don't lose the broadcast.
	m_mutex->unlock();

	// wait for a signal or broadcast
	DWORD result = WaitForMultipleObjects(n, handles, FALSE, winTimeout);

	// cancel takes priority
	if (n == 3 && result != WAIT_OBJECT_0 + 2 &&
					WaitForSingleObject(handles[2], 0) == WAIT_OBJECT_0) {
		result = WAIT_OBJECT_0 + 2;
	}

	// update the waiter count and check if we're the last waiter
	bool last;
	{
		CLock lock(&m_waitCountMutex);
		--m_waitCount;
		last = (result == WAIT_OBJECT_0 + 1 && m_waitCount == 0);
	}

	// reset the broadcast event if we're the last waiter
	if (last) {
		ResetEvent(events[kBroadcast]);
	}

	// reacquire the mutex
	m_mutex->lock();

	// cancel thread if necessary
	if (result == WAIT_OBJECT_0 + 2) {
		currentRep->testCancel();
	}

	// return success or failure
	return (result == WAIT_OBJECT_0 + 0 ||
			result == WAIT_OBJECT_0 + 1);
}

#endif // CONFIG_PLATFORM_WIN32

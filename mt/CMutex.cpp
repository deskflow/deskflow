#include "CMutex.h"
#include "CLog.h"
#include <assert.h>

//
// CMutex
//

CMutex::CMutex()
{
	init();
}

CMutex::CMutex(const CMutex&)
{
	init();
}

CMutex::~CMutex()
{
	fini();
}

CMutex&					CMutex::operator=(const CMutex&)
{
	return *this;
}

#if defined(CONFIG_PTHREADS)

#include <pthread.h>
#include <errno.h>

void					CMutex::init()
{
	pthread_mutex_t* mutex = new pthread_mutex_t;
	int status = pthread_mutex_init(mutex, NULL);
	assert(status == 0);
//	status = pthread_mutexattr_settype(mutex, PTHREAD_MUTEX_RECURSIVE);
//	assert(status == 0);
	m_mutex = reinterpret_cast<void*>(mutex);
}

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
void					CMutex::fini()
{
	pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_mutex);
	int status = pthread_mutex_destroy(mutex);
	logc(status != 0, (CLOG_ERR "pthread_mutex_destroy status %d", status));
	assert(status == 0);
	delete mutex;
}

void					CMutex::lock() const
{
	pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_mutex);
	int status = pthread_mutex_lock(mutex);

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
		log((CLOG_ERR "pthread_mutex_lock status %d", status));
		assert(0 && "unexpected error");
	}
}

void					CMutex::unlock() const
{
	pthread_mutex_t* mutex = reinterpret_cast<pthread_mutex_t*>(m_mutex);
	int status = pthread_mutex_unlock(mutex);

	switch (status) {
	  case 0:
		// success
		return;

	  case EPERM:
		assert(0 && "thread doesn't own a lock");
		break;

	  default:
		log((CLOG_ERR "pthread_mutex_unlock status %d", status));
		assert(0 && "unexpected error");
	}
}

#endif // CONFIG_PTHREADS

#if defined(CONFIG_PLATFORM_WIN32)

#include <windows.h>

void					CMutex::init()
{
	CRITICAL_SECTION* mutex = new CRITICAL_SECTION;
	::InitializeCriticalSection(mutex);
	m_mutex = reinterpret_cast<void*>(mutex);
}

void					CMutex::fini()
{
	CRITICAL_SECTION* mutex = reinterpret_cast<CRITICAL_SECTION*>(m_mutex);
	::DeleteCriticalSection(mutex);
	delete mutex;
}

void					CMutex::lock() const
{
	::EnterCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(m_mutex));
}

void					CMutex::unlock() const
{
	::LeaveCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(m_mutex));
}

#endif // CONFIG_PLATFORM_WIN32

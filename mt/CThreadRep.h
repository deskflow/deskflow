#ifndef CTHREADREP_H
#define CTHREADREP_H

#include "BasicTypes.h"

#if defined(CONFIG_PTHREADS)
#include <pthread.h>
#elif defined(CONFIG_PLATFORM_WIN32)
#include <windows.h>
#endif

class CMutex;
class IJob;

class CThreadRep {
  public:
	CThreadRep(IJob*, void* userData);

	// manipulators

	// initialize the thread library
	static void			initThreads();

	// change ref count
	void				ref();
	void				unref();

    // the calling thread sleeps for t seconds.  if t == 0.0 then
    // the thread yields the CPU.
    void				sleep(double timeout);

	// cancel the thread
	void				cancel();

	// set cancellation state
	bool				enableCancel(bool enable);

	// permanently disable further cancellation and start cancel cleanup
	// if cancel has been called and cancellation hasn't been started yet.
	void				testCancel();

	// wait for thread to exit or for current thread to cancel
	bool				wait(CThreadRep*, double timeout);

    // set the priority
    void				setPriority(int n);

    // accessors

    // get the exit result for this thread.  thread must be terminated.
    void*				getResult() const;

    // get the user data passed to the constructor
    void*				getUserData() const;

	// get the current cancellable state
	bool				isCancellable() const;

#if defined(CONFIG_PTHREADS)
	bool				isExited() const;
#elif defined(CONFIG_PLATFORM_WIN32)
	HANDLE				getExitEvent() const;
	HANDLE				getCancelEvent() const;
#endif

    // return the thread rep for the calling thread.  the returned
    // rep has been ref()'d.
    static CThreadRep*	getCurrentThreadRep();

  protected:
	virtual ~CThreadRep();

  private:
	// internal constructor
	CThreadRep();

	// initialization/cleanup
	void				init();
	void				fini();

	// thread rep lookup
	static CThreadRep*	find();

	// thread functions
#if defined(CONFIG_PTHREADS)
	static void*		threadFunc(void* arg);
	static void			threadCancel(int);
#elif defined(CONFIG_PLATFORM_WIN32)
	static unsigned int __stdcall	threadFunc(void* arg);
#endif
	void				doThreadFunc();

	// not implemented
	CThreadRep(const CThreadRep&);
	CThreadRep& operator=(const CThreadRep&);

  private:
	static CMutex*		s_mutex;
	static CThreadRep*	s_head;

	CThreadRep*			m_prev;
	CThreadRep*			m_next;

	SInt32				m_refCount;
	IJob*				m_job;
	void*				m_userData;
	void*				m_result;
	bool				m_cancellable;
	bool				m_cancelling;

#if defined(CONFIG_PTHREADS)
	pthread_t			m_thread;
	bool				m_exit;
	bool				m_cancel;
#endif

#if defined(CONFIG_PLATFORM_WIN32)
	HANDLE				m_thread;
	DWORD				m_id;
	HANDLE				m_exit;
	HANDLE				m_cancel;
#endif
};

//
// CThreadPtr -- auto unref'ing pointer to thread rep
//

class CThreadPtr {
  public:
	CThreadPtr(CThreadRep* rep) : m_rep(rep) { }
	~CThreadPtr() { m_rep->unref(); }

	CThreadRep*			operator->() const { return m_rep; }

  private:
	// not implemented
	CThreadPtr(const CThreadPtr&);
	CThreadPtr& operator=(const CThreadPtr&);

  private:
	CThreadRep*			m_rep;
};

#endif

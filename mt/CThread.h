#ifndef CTHREAD_H
#define CTHREAD_H

#include "common.h"

class IJob;
class CThreadRep;

// note -- do not derive from this class
class CThread {
  public:
    // create and start a new thread executing the job.
	// the user data can be retrieved with getUserData().
    CThread(IJob* adopted, void* userData = 0);

    // make a new thread object that refers to an existing thread.
	// this does *not* start a new thread.
    CThread(const CThread&);

    // release thread.  this does not terminate the thread.  a thread
    // will keep running until the job completes or calls exit().
    ~CThread();

    // manipulators

    // assign thread.  this has no effect on the threads.  it simply
    // makes this thread object refer to another thread.  it does *not*
	// start a new thread.
    CThread&			operator=(const CThread&);

    // the calling thread sleeps for the given number of seconds.  if
    // timeout <= 0.0 then the call returns immediately.  if timeout
	// == 0.0 then the calling thread yields the CPU.
	// (cancellation point)
    static void			sleep(double timeout);

    // terminate the calling thread.  this function does not return but
    // the stack is unwound and automatic objects are destroyed, as if
    // exit() threw an exception (which is, in fact, what it does).  the
    // argument is saved as the result returned by getResult().  if you
    // have a catch(...) block then you should add the following before
    // it to avoid catching the exit:  catch(CThreadExit&) { throw; }
    static void			exit(void*);

	// enable/disable cancellation.  default is enabled.  this is not
	// a cancellation point so if you enabled cancellation and want to
	// allow immediate cancellation you need to call testCancel().
	// return value is the previous state.
	static bool			enableCancel(bool);

	// cancel the thread.  cancel() never waits for the thread to
	// terminate;  it just posts the cancel and returns.  a thread will
	// terminate when it enters a cancellation point with cancellation
	// enabled.  if cancellation is disabled then the cancel is
	// remembered but not acted on until the first call to a
	// cancellation point after cancellation is enabled.
	//
	// a cancellation point is a function that can act on cancellation.
	// a cancellation point does not return if there's a cancel pending.
	// instead, it unwinds the stack and destroys automatic objects, as
	// if cancel() threw an exception (which is, in fact, what it does).
	// threads must take care to clean up and release any resources they
	// may have, especially mutexes.  they can catch (XThreadCancel) to
	// do that then rethrow the exception or they can let it happen
	// automatically by doing clean up in the d'tors of automatic
	// objects.  clients are strongly encouraged to do the latter.
	// during cancellation, further cancel() calls are ignored (i.e.
	// a thread cannot be interrupted by a cancel during cancellation).
	//
	// clients that catch (XThreadCancel) must always rethrow the
	// exception.  clients that catch(...) must either rethrow the
	// exception or include a catch (XThreadCancel) handler that
	// rethrows.
    void				cancel();

    // change the priority of the thread.  normal priority is 0, 1 is
	// the next lower, etc.  -1 is the next higher, etc. but boosting
	// the priority may not be available.
    void				setPriority(int n);

    // accessors

    // return a thread object representing the calling thread
    static CThread		getCurrentThread();

    // get the user data passed to the constructor for the current
	// thread.
    static void*		getUserData();

    // testCancel() does nothing but is a cancellation point.  call
	// this to make a function itself a cancellation point.
	// (cancellation point)
    static void			testCancel();

	// waits for the thread to terminate (by exit() or cancel() or
	// by returning from the thread job).  returns immediately if
	// the thread has already terminated.  returns immediately with
	// false if called by a thread on itself.  returns false on
	// timeout (or error) and true on success.
	// (cancellation point)
	bool				wait(double timeout = -1.0) const;

    // get the exit result.  does an implicit wait().  returns NULL
	// immediately if called by a thread on itself.  returns NULL for
	// threads that were cancelled.
	// (cancellation point)
	void*				getResult() const;

    // compare threads for (in)equality
    bool				operator==(const CThread&) const;
    bool				operator!=(const CThread&) const;

  private:
    CThread(CThreadRep*);

  private:
    CThreadRep*			m_rep;
};

// disables cancellation in the c'tor and enables it in the d'tor.
class CThreadMaskCancel {
  public:
	CThreadMaskCancel() : m_old(CThread::enableCancel(false)) { }
	~CThreadMaskCancel() { CThread::enableCancel(m_old); }

  private:
	bool				m_old;
};

#endif

#ifndef CTIMERTHREAD_H
#define CTIMERTHREAD_H

class CThread;

//! A timer thread
/*!
An object of this class cancels the thread that called the c'tor unless
the object is destroyed before a given timeout.
*/
class CTimerThread {
public:
	//! Cancel calling thread after \c timeout seconds
	/*!
	Cancels the calling thread after \c timeout seconds unless destroyed
	before then.  If \c timeout is less than zero then it never times
	out and this is a no-op.
	*/
	CTimerThread(double timeout);
	//! Cancel the timer thread
	~CTimerThread();

private:
	void				timer(void*);

	// not implemented
	CTimerThread(const CTimerThread&);
	CTimerThread& operator=(const CTimerThread&);

private:
	double				m_timeout;
	CThread*			m_callingThread;
	CThread*			m_timingThread;
};

#endif


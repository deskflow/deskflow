#ifndef CTIMERTHREAD_H
#define CTIMERTHREAD_H

class CThread;

class CTimerThread {
public:
	// cancels the calling thread after timeout seconds unless destroyed
	// before then.  if timeout is less than zero then it never times
	// out and is a no-op.
	CTimerThread(double timeout);
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


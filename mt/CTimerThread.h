#ifndef CTIMERTHREAD_H
#define CTIMERTHREAD_H

#include "common.h"

class CThread;

class CTimerThread {
  public:
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


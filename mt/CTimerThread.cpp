#include "CTimerThread.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CLog.h"
#include <assert.h>

//
// CTimerThread
//

CTimerThread::CTimerThread(double timeout) : m_timeout(timeout)
{
	assert(m_timeout > 0.0);
	m_callingThread = new CThread(CThread::getCurrentThread());
	m_timingThread  = new CThread(new TMethodJob<CTimerThread>(
								this, &CTimerThread::timer));
}

CTimerThread::~CTimerThread()
{
	m_timingThread->cancel();
	delete m_timingThread;
	delete m_callingThread;
}

void					CTimerThread::timer(void*)
{
	log((CLOG_DEBUG "timeout in %f seconds", m_timeout));
	CThread::sleep(m_timeout);
	log((CLOG_DEBUG "timeout"));
	m_callingThread->cancel();
}


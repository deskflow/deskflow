#include "CTimerThread.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CLog.h"

//
// CTimerThread
//

CTimerThread::CTimerThread(double timeout) : m_timeout(timeout)
{
	if (m_timeout >= 0.0) {
		m_callingThread = new CThread(CThread::getCurrentThread());
		m_timingThread  = new CThread(new TMethodJob<CTimerThread>(
								this, &CTimerThread::timer));
	}
	else {
		m_callingThread = NULL;
		m_timingThread  = NULL;
	}
}

CTimerThread::~CTimerThread()
{
	if (m_timingThread != NULL) {
		log((CLOG_DEBUG1 "cancelling timeout"));
		m_timingThread->cancel();
		m_timingThread->wait();
		log((CLOG_DEBUG1 "cancelled timeout"));
		delete m_timingThread;
		delete m_callingThread;
	}
}

void
CTimerThread::timer(void*)
{
	log((CLOG_DEBUG1 "timeout in %f seconds", m_timeout));
	CThread::sleep(m_timeout);
	log((CLOG_DEBUG1 "timeout"));
	m_callingThread->cancel();
}

#include "CThread.h"
#include "CThreadRep.h"
#include "XThread.h"
#include "CLock.h"
#include "CStopwatch.h"

//
// CThreadPtr
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

//
// CThread
//

CThread::CThread(IJob* job, void* userData)
{
	m_rep = new CThreadRep(job, userData);
}

CThread::CThread(const CThread& thread) : m_rep(thread.m_rep)
{
	m_rep->ref();
}

CThread::CThread(CThreadRep* rep) : m_rep(rep)
{
	// do nothing.  rep should have already been Ref()'d.
}

CThread::~CThread()
{
	m_rep->unref();
}

CThread&				CThread::operator=(const CThread& thread)
{
	if (thread.m_rep != m_rep) {
		m_rep->unref();
		m_rep = thread.m_rep;
		m_rep->ref();
	}
	return *this;
}

void					CThread::sleep(double timeout)
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	if (timeout >= 0.0) {
		currentRep->testCancel();
		currentRep->sleep(timeout);
	}
	currentRep->testCancel();
}

void					CThread::exit(void* result)
{
	throw XThreadExit(result);
}

bool					CThread::enableCancel(bool enable)
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	return currentRep->enableCancel(enable);
}

void					CThread::cancel()
{
	m_rep->cancel();
}

void					CThread::setPriority(int n)
{
	m_rep->setPriority(n);
}

CThread					CThread::getCurrentThread()
{
	return CThread(CThreadRep::getCurrentThreadRep());
}

bool					CThread::wait(double timeout) const
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	return currentRep->wait(m_rep, timeout);
}

void					CThread::testCancel()
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	currentRep->testCancel();
}

void*					CThread::getResult() const
{
	if (wait())
		return m_rep->getResult();
	else
		return NULL;
}

void*					CThread::getUserData()
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	return currentRep->getUserData();
}

bool					CThread::operator==(const CThread& thread) const
{
	return (m_rep == thread.m_rep);
}

bool					CThread::operator!=(const CThread& thread) const
{
	return (m_rep != thread.m_rep);
}

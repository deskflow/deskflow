#include "CLock.h"
#include "CMutex.h"
#include "CCondVar.h"

//
// CLock
//

CLock::CLock(const CMutex* mutex) throw() : m_mutex(mutex)
{
	m_mutex->lock();
}

CLock::CLock(const CCondVarBase* cv) throw() : m_mutex(cv->getMutex())
{
	m_mutex->lock();
}

CLock::~CLock() throw()
{
	m_mutex->unlock();
}

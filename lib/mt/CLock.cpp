#include "CLock.h"
#include "CCondVar.h"
#include "CMutex.h"

//
// CLock
//

CLock::CLock(const CMutex* mutex) :
	m_mutex(mutex)
{
	m_mutex->lock();
}

CLock::CLock(const CCondVarBase* cv) :
	m_mutex(cv->getMutex())
{
	m_mutex->lock();
}

CLock::~CLock()
{
	m_mutex->unlock();
}

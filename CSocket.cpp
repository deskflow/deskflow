#include "CSocket.h"
#include "IJob.h"

//
// CSocket
//

CSocket::CSocket() : m_readJob(NULL), m_writeJob(NULL)
{
	// do nothing
}

CSocket::~CSocket()
{
	delete m_readJob;
	delete m_writeJob;
}

void					CSocket::setReadJob(IJob* adoptedJob)
{
	delete m_readJob;
	m_readJob = adoptedJob;
	onJobChanged();
}

void					CSocket::setWriteJob(IJob* adoptedJob)
{
	delete m_writeJob;
	m_writeJob = adoptedJob;
	onJobChanged();
}

void					CSocket::onJobChanged()
{
	// do nothing
}

void					CSocket::runReadJob()
{
	if (m_readJob)
		m_readJob->run();
}

void					CSocket::runWriteJob()
{
	if (m_writeJob)
		m_writeJob->run();
}

bool					CSocket::hasReadJob() const
{
	return (m_readJob != NULL);
}

bool					CSocket::hasWriteJob() const
{
	return (m_writeJob != NULL);
}

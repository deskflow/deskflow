#include "CUnixEventQueue.h"
#include "IJob.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

//
// CUnixEventQueue
//

CUnixEventQueue::CUnixEventQueue()
{
	// do nothing
}

CUnixEventQueue::~CUnixEventQueue()
{
	// clean up lists
	clearList(m_readList);
	clearList(m_writeList);
}

void					CUnixEventQueue::addFileDesc(int fd,
								IJob* readJob, IJob* writeJob)
{
	assert(fd != -1);
	assert(m_readList.count(fd) == 0 && m_writeList.count(fd) == 0);
	assert(readJob != writeJob || readJob == NULL);

	if (readJob)
		m_readList[fd] = readJob;
	if (writeJob)
		m_writeList[fd] = writeJob;
}

void					CUnixEventQueue::removeFileDesc(int fd)
{
	assert(fd != -1);

	// remove from lists
	eraseList(m_readList, fd);
	eraseList(m_writeList, fd);
}

void					CUnixEventQueue::wait(double timeout)
{
	// prepare sets
	fd_set fdRead, fdWrite;
	const int maxRead  = prepList(m_readList,  &fdRead);
	const int maxWrite = prepList(m_writeList, &fdWrite);

	// compute the larger of maxRead and maxWrite
	const int fdMax = (maxRead > maxWrite) ? maxRead : maxWrite;
	if (fdMax == -1)
		return;

	// prepare timeout
	struct timeval* pTimeout = NULL;
	struct timeval  sTimeout;
	if (timeout >= 0.0) {
		sTimeout.tv_sec  = static_cast<int>(timeout);
		sTimeout.tv_usec = static_cast<int>(1000000.0 *
								(timeout - sTimeout.tv_sec));
		pTimeout = &sTimeout;
	}

	// wait
	const int n = ::select(fdMax + 1, &fdRead, &fdWrite, NULL, pTimeout);

	// return on error or if nothing to do
	if (n <= 0)
		return;

	// invoke jobs
	// note -- calling removeFileDesc() from a job is likely to crash the
	// program because we expect all jobs with active file descriptors to
	// persist for the duration of these loops.
	int fd;
	for (fd = 0; fd <= maxRead; ++fd)
		if (FD_ISSET(fd, &fdRead)) {
			assert(m_readList.count(fd) > 0);
			assert(m_readList[fd] != NULL);
			m_readList[fd]->run();
		}
	for (fd = 0; fd <= maxWrite; ++fd)
		if (FD_ISSET(fd, &fdWrite)) {
			assert(m_writeList.count(fd) > 0);
			assert(m_writeList[fd] != NULL);
			m_writeList[fd]->run();
		}
}

void					CUnixEventQueue::lock()
{
	// do nothing
}

void					CUnixEventQueue::unlock()
{
	// do nothing
}

void					CUnixEventQueue::signalNotEmpty()
{
	// do nothing
}

void					CUnixEventQueue::eraseList(List& list, int fd) const
{
	List::iterator index = list.find(fd);
	if (index != list.end()) {
		delete index->second;
		list.erase(index);
	}
}

void					CUnixEventQueue::clearList(List& list) const
{
	for (List::const_iterator index = list.begin();
								index != list.end(); ++index)
		delete index->second;
	list.clear();
}

int					CUnixEventQueue::prepList(
								const List& list, void* vfdSet) const
{
	fd_set* fdSet = reinterpret_cast<fd_set*>(vfdSet);
	FD_ZERO(fdSet);

	int fdMax = -1;
	for (List::const_iterator index = list.begin();
								index != list.end(); ++index) {
		const int fd = index->first;
		FD_SET(fd, fdSet);
		if (fd > fdMax)
			fdMax = fd;
	}

	return fdMax;
}

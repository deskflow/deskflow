#ifndef CUNIXEVENTQUEUE_H
#define CUNIXEVENTQUEUE_H

#include "CEventQueue.h"
#include <map>

#undef CEQ
#define CEQ ((CUnixEventQueue*)CEventQueue::getInstance())

class IJob;

class CUnixEventQueue : public CEventQueue {
  public:
	CUnixEventQueue();
	virtual ~CUnixEventQueue();

	// manipulators

	// add a file descriptor to wait on.  if adoptedReadJob is not NULL
	// then it'll be called when the file descriptor is readable.  if
	// adoptedWriteJob is not NULL then it will be called then the file
	// descriptor is writable.  at least one job must not be NULL and
	// the jobs may not be the same.  ownership of the jobs is assumed.
	// the file descriptor must not have already been added or, if it
	// was, it must have been removed.
	void				addFileDesc(int fd,
								IJob* adoptedReadJob, IJob* adoptedWriteJob);

	// remove a file descriptor from the list being waited on.  the
	// associated jobs are destroyed.  the file descriptor must have
	// been added and not since removed.
	void				removeFileDesc(int fd);

	// IEventQueue overrides
	virtual void		wait(double timeout);

  protected:
	// CEventQueue overrides
	virtual void		lock();
	virtual void		unlock();
	virtual void		signalNotEmpty();

  private:
	typedef std::map<int, IJob*> List;
	void				eraseList(List&, int fd) const;
	void				clearList(List&) const;
	int					prepList(const List&, void* fdSet) const;

  private:
	List				m_readList;
	List				m_writeList;
};

#endif

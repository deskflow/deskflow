#ifndef CEVENTQUEUE_H
#define CEVENTQUEUE_H

#include "IEventQueue.h"
#include "CEvent.h"
#include <list>

class CEventQueue : public IEventQueue {
  public:
	CEventQueue();
	virtual ~CEventQueue();

	// IEventQueue overrides
	virtual void		wait(double timeout) = 0;
	virtual void		pop(CEvent*);
	virtual void		push(const CEvent*);
	virtual bool		isEmpty();

  protected:
	// signal the queue not-empty condition.  this should cause wait()
	// to stop waiting.
	virtual void		signalNotEmpty() = 0;

	// lock the queue mutex
	virtual void		lock() = 0;

	// unlock the queue mutex
	virtual void		unlock() = 0;

  private:
	typedef std::list<CEvent> List;

	List				m_queue;
};

#endif

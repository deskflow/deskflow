#ifndef IEVENTQUEUE_H
#define IEVENTQUEUE_H

#define CEQ (IEventQueue::getInstance())

class CEvent;

class IEventQueue {
  public:
	IEventQueue();
	virtual ~IEventQueue();

	// note -- all of the methods in an IEventQueue subclass for a
	// platform must be thread safe if it will be used by multiple
	// threads simultaneously on that platform.

	// manipulators

	// wait up to timeout seconds for the queue to become not empty.
	// as a side effect this can do the insertion of events.  if
	// timeout < 0.0 then wait indefinitely.  it's possible for
	// wait() to return prematurely so always call isEmpty() to
	// see if there are any events.
	virtual void		wait(double timeout) = 0;

	// reads and removes the next event on the queue.  waits indefinitely
	// for an event if the queue is empty.
	virtual void		pop(CEvent*) = 0;

	// push an event onto the queue
	virtual void		push(const CEvent*) = 0;

	// returns true if the queue is empty and wait() would block
	virtual bool		isEmpty() = 0;

	// accessors

	// get the singleton event queue
	static IEventQueue*	getInstance();

  private:
	static IEventQueue*	s_instance;
};

#endif

#include "CEventQueue.h"

//
// IEventQueue
//

IEventQueue*			IEventQueue::s_instance = NULL;

IEventQueue::IEventQueue()
{
	assert(s_instance == NULL);
	s_instance = this;
}

IEventQueue::~IEventQueue()
{
	s_instance = NULL;
}

IEventQueue*			IEventQueue::getInstance()
{
	return s_instance;
}


//
// CEventQueue
//

CEventQueue::CEventQueue()
{
	// do nothing
}

CEventQueue::~CEventQueue()
{
	// do nothing
}

void					CEventQueue::pop(CEvent* event)
{
	assert(event != NULL);

	// wait for an event
	while (isEmpty())
		wait(-1.0);

	// lock the queue, extract an event, then unlock
	lock();
	*event = m_queue.front();
	m_queue.pop_front();
	unlock();
}

void					CEventQueue::push(const CEvent* event)
{
	// put the event at the end of the queue and signal that the queue
	// is not empty
	lock();
	m_queue.push_back(*event);
	signalNotEmpty();
	unlock();
}

bool					CEventQueue::isEmpty()
{
	lock();
	bool e = m_queue.empty();
	unlock();

	// if queue is empty then poll to see if more stuff is ready to go
	// on the queue and check again if the queue is empty.
	if (e) {
		wait(0.0);
		lock();
		e = m_queue.empty();
		unlock();
	}
	return e;
}

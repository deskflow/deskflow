/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CEVENTQUEUE_H
#define CEVENTQUEUE_H

#include "IEventQueue.h"
#include "CEvent.h"
#include "CPriorityQueue.h"
#include "CStopwatch.h"
#include "IArchMultithread.h"
#include "stdmap.h"
#include "stdset.h"

//! Event queue
/*!
An event queue that implements the platform independent parts and
delegates the platform dependent parts to a subclass.
*/
class CEventQueue : public IEventQueue {
public:
	CEventQueue();
	virtual ~CEventQueue();

	// IEventQueue overrides
	virtual bool		getEvent(CEvent& event, double timeout = -1.0);
	virtual bool		dispatchEvent(const CEvent& event);
	virtual void		addEvent(const CEvent& event);
	virtual CEventQueueTimer*
						newTimer(double duration, void* target = NULL);
	virtual CEventQueueTimer*
						newOneShotTimer(double duration, void* target = NULL);
	virtual void		deleteTimer(CEventQueueTimer*);
	virtual void		adoptHandler(void* target, IEventJob* dispatcher);
	virtual void		adoptHandler(CEvent::Type type,
							void* target, IEventJob* handler);
	virtual IEventJob*	orphanHandler(void* target);
	virtual IEventJob*	orphanHandler(CEvent::Type type, void* target);
	virtual void		removeHandler(void* target);
	virtual void		removeHandler(CEvent::Type type, void* target);
	virtual bool		isEmpty() const;
	virtual IEventJob*	getHandler(CEvent::Type type, void* target) const;

protected:
	//! @name manipulators
	//@{

	//! Get the data for a given id
	/*!
	Takes a saved event id, \p eventID, and returns a \c CEvent.  The
	event id becomes invalid after this call.  The \p eventID must have
	been passed to a successful call to \c doAddEvent() and not removed
	since.
	*/
	CEvent				removeEvent(UInt32 eventID);

	//! Block waiting for an event
	/*!
	Wait for an event in the system event queue for up to \p timeout
	seconds.
	*/
	virtual void		waitForEvent(double timeout) = 0;

	//! Get the next event
	/*!
	Remove the next system event (one should be pending) and convert it
	to a \c CEvent.  The event type should be either \c CEvent::kSystem
	if the event was not added by \c doAddEvent() or a type returned by
	\c CEvent::registerType() if it was (and not \c CEvent::kTimer).  A
	non-system event will normally be retrieved by \c removeEvent(), but
	the implementation must be able to tell the difference between a
	system event and one added by \c doAddEvent().
	*/
	virtual bool		doGetEvent(CEvent& event) = 0;

	//! Post an event
	/*!
	Add the given event to the end of the system queue.  This is a user
	event and \c doGetEvent() must be able to identify it as such.
	This method must cause \c waitForEvent() to return at some future
	time if it's blocked waiting on an event.
	*/
	virtual bool		doAddEvent(UInt32 dataID) = 0;

	//@}
	//! @name accessors
	//@{

	//! Check if system queue is empty
	/*!
	Return true iff the system queue is empty.
	*/
	virtual bool		doIsEmpty() const = 0;

	//! Create a timer object
	/*!
	Create and return a timer object.  The object is opaque and is
	used only by the subclass but it must be a valid object (i.e.
	not NULL).
	*/
	virtual CEventQueueTimer*
						doNewTimer(double duration, bool oneShot) const = 0;

	//! Destroy a timer object
	/*!
	Destroy a timer object previously returned by \c doNewTimer().
	*/
	virtual void		doDeleteTimer(CEventQueueTimer*) const = 0;

	//@}

private:
	void				doAdoptHandler(CEvent::Type type,
							void* target, IEventJob* handler);
	IEventJob*			doOrphanHandler(CEvent::Type type, void* target);

	UInt32				saveEvent(const CEvent& event);
	bool				hasTimerExpired(CEvent& event);
	double				getNextTimerTimeout() const;

private:
	class CTypeTarget {
	public:
		CTypeTarget(CEvent::Type type, void* target);
		~CTypeTarget();

		bool			operator<(const CTypeTarget&) const;

	private:
		CEvent::Type	m_type;
		void*			m_target;
	};
	class CTimer {
	public:
		CTimer(CEventQueueTimer*, double timeout, void* target, bool oneShot);
		~CTimer();

		void			reset();

		CTimer&			operator-=(double);

						operator double() const;

		bool			isOneShot() const;
		CEventQueueTimer*
						getTimer() const;
		void*			getTarget() const;
		void			fillEvent(CTimerEvent&) const;

		bool			operator<(const CTimer&) const;

	private:
		CEventQueueTimer*	m_timer;
		double				m_timeout;
		void*				m_target;
		bool				m_oneShot;
		double				m_time;
	};
	typedef std::set<CEventQueueTimer*> CTimers;
	typedef CPriorityQueue<CTimer> CTimerQueue;
	typedef std::map<UInt32, CEvent> CEventTable;
	typedef std::vector<UInt32> CEventIDList;
	typedef std::map<CTypeTarget, IEventJob*> CHandlerTable;

	CArchMutex			m_mutex;

	CEventTable			m_events;
	CEventIDList		m_oldEventIDs;

	CStopwatch			m_time;
	CTimers				m_timers;
	CTimerQueue			m_timerQueue;
	CTimerEvent			m_timerEvent;

	CHandlerTable		m_handlers;
};

#endif

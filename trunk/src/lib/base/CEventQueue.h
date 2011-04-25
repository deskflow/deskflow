/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
	virtual void		adoptBuffer(IEventQueueBuffer*);
	virtual bool		getEvent(CEvent& event, double timeout = -1.0);
	virtual bool		dispatchEvent(const CEvent& event);
	virtual void		addEvent(const CEvent& event);
	virtual CEventQueueTimer*
						newTimer(double duration, void* target);
	virtual CEventQueueTimer*
						newOneShotTimer(double duration, void* target);
	virtual void		deleteTimer(CEventQueueTimer*);
	virtual void		adoptHandler(CEvent::Type type,
							void* target, IEventJob* handler);
	virtual void		removeHandler(CEvent::Type type, void* target);
	virtual void		removeHandlers(void* target);
	virtual CEvent::Type
						registerType(const char* name);
	virtual CEvent::Type
						registerTypeOnce(CEvent::Type& type, const char* name);
	virtual bool		isEmpty() const;
	virtual IEventJob*	getHandler(CEvent::Type type, void* target) const;
	virtual const char*	getTypeName(CEvent::Type type);

private:
	UInt32				saveEvent(const CEvent& event);
	CEvent				removeEvent(UInt32 eventID);
	bool				hasTimerExpired(CEvent& event);
	double				getNextTimerTimeout() const;

private:
	class CTimer {
	public:
		CTimer(CEventQueueTimer*, double timeout, double initialTime,
							void* target, bool oneShot);
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
	typedef std::map<CEvent::Type, const char*> CTypeMap;
	typedef std::map<CEvent::Type, IEventJob*> CTypeHandlerTable;
	typedef std::map<void*, CTypeHandlerTable> CHandlerTable;

	CArchMutex			m_mutex;

	// registered events
	CEvent::Type		m_nextType;
	CTypeMap			m_typeMap;

	// buffer of events
	IEventQueueBuffer*	m_buffer;

	// saved events
	CEventTable			m_events;
	CEventIDList		m_oldEventIDs;

	// timers
	CStopwatch			m_time;
	CTimers				m_timers;
	CTimerQueue			m_timerQueue;
	CTimerEvent			m_timerEvent;

	// event handlers
	CHandlerTable		m_handlers;
};

#endif

#ifndef CSTOPWATCH_H
#define CSTOPWATCH_H

#include "common.h"

class CStopwatch {
public:
	// the default constructor does an implicit reset() or setTrigger().
	// if triggered == false then the clock starts ticking.
	CStopwatch(bool triggered = false);
	~CStopwatch();

	// manipulators

	// set the start time to the current time, returning the time since
	// the last reset.  this does not remove the trigger if it's set nor
	// does it start a stopped clock.  if the clock is stopped then
	// subsequent reset()'s will return 0.
	double				reset();

	// stop and start the stopwatch.  while stopped, no time elapses.
	// stop() does not remove the trigger but start() does, even if
	// the clock was already started.
	void				stop();
	void				start();

	// setTrigger() stops the clock like stop() except there's an
	// implicit start() the next time (non-const) getTime() is called.
	// this is useful when you want the clock to start the first time
	// you check it.
	void				setTrigger();

	// return the time since the last reset() (or call reset() and
	// return zero if the trigger is set).
	double				getTime();
						operator double();

	// accessors

	// returns true if the watch is stopped
	bool				isStopped() const;

	// return the time since the last reset().  these cannot trigger
	// the clock to start so if the trigger is set it's as if it wasn't.
	double				getTime() const;
						operator double() const;

private:
	double				getClock() const;

private:
	double				m_mark;
	bool				m_triggered;
	bool				m_stopped;
};

#endif

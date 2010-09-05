/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
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

#include "CStopwatch.h"
#include "CArch.h"

//
// CStopwatch
//

CStopwatch::CStopwatch(bool triggered) :
	m_mark(0.0),
	m_triggered(triggered),
	m_stopped(triggered)
{
	if (!triggered) {
		m_mark = ARCH->time();
	}
}

CStopwatch::~CStopwatch()
{
	// do nothing
}

double
CStopwatch::reset()
{
	if (m_stopped) {
		const double dt = m_mark;
		m_mark = 0.0;
		return dt;
	}
	else {
		const double t	= ARCH->time();
		const double dt = t - m_mark;
		m_mark = t;
		return dt;
	}
}

void
CStopwatch::stop()
{
	if (m_stopped) {
		return;
	}

	// save the elapsed time
	m_mark	  = ARCH->time() - m_mark;
	m_stopped = true;
}

void
CStopwatch::start()
{
	m_triggered = false;
	if (!m_stopped) {
		return;
	}

	// set the mark such that it reports the time elapsed at stop()
	m_mark	  = ARCH->time() - m_mark;
	m_stopped = false;
}

void
CStopwatch::setTrigger()
{
	stop();
	m_triggered = true;
}

double
CStopwatch::getTime()
{
	if (m_triggered) {
		const double dt = m_mark;
		start();
		return dt;
	}
	else if (m_stopped) {
		return m_mark;
	}
	else {
		return ARCH->time() - m_mark;
	}
}

CStopwatch::operator double()
{
	return getTime();
}

bool
CStopwatch::isStopped() const
{
	return m_stopped;
}

double
CStopwatch::getTime() const
{
	if (m_stopped) {
		return m_mark;
	}
	else {
		return ARCH->time() - m_mark;
	}
}

CStopwatch::operator double() const
{
	return getTime();
}

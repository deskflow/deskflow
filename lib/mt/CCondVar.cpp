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

#include "CCondVar.h"
#include "CStopwatch.h"
#include "CArch.h"

//
// CCondVarBase
//

CCondVarBase::CCondVarBase(CMutex* mutex) : 
	m_mutex(mutex)
{
	assert(m_mutex != NULL);
	m_cond = ARCH->newCondVar();
}

CCondVarBase::~CCondVarBase()
{
	ARCH->closeCondVar(m_cond);
}

void
CCondVarBase::lock() const
{
	m_mutex->lock();
}

void
CCondVarBase::unlock() const
{
	m_mutex->unlock();
}

void
CCondVarBase::signal()
{
	ARCH->signalCondVar(m_cond);
}

void
CCondVarBase::broadcast()
{
	ARCH->broadcastCondVar(m_cond);
}

bool
CCondVarBase::wait(CStopwatch& timer, double timeout) const
{
	// check timeout against timer
	if (timeout >= 0.0) {
		timeout -= timer.getTime();
		if (timeout < 0.0)
			return false;
	}
	return wait(timeout);
}

bool
CCondVarBase::wait(double timeout) const
{
	return ARCH->waitCondVar(m_cond, m_mutex->m_mutex, timeout);
}

CMutex*
CCondVarBase::getMutex() const
{
	return m_mutex;
}

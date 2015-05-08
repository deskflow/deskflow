/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mt/CondVar.h"
#include "arch/Arch.h"
#include "base/Stopwatch.h"

//
// CondVarBase
//

CondVarBase::CondVarBase(Mutex* mutex) : 
	m_mutex(mutex)
{
	assert(m_mutex != NULL);
	m_cond = ARCH->newCondVar();
}

CondVarBase::~CondVarBase()
{
	ARCH->closeCondVar(m_cond);
}

void
CondVarBase::lock() const
{
	m_mutex->lock();
}

void
CondVarBase::unlock() const
{
	m_mutex->unlock();
}

void
CondVarBase::signal()
{
	ARCH->signalCondVar(m_cond);
}

void
CondVarBase::broadcast()
{
	ARCH->broadcastCondVar(m_cond);
}

bool
CondVarBase::wait(Stopwatch& timer, double timeout) const
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
CondVarBase::wait(double timeout) const
{
	return ARCH->waitCondVar(m_cond, m_mutex->m_mutex, timeout);
}

Mutex*
CondVarBase::getMutex() const
{
	return m_mutex;
}

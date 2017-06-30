/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

CondVarBase::CondVarBase (Mutex* mutex) : m_mutex (mutex) {
    assert (m_mutex != NULL);
    m_cond = ARCH->newCondVar ();
}

CondVarBase::~CondVarBase () {
    ARCH->closeCondVar (m_cond);
}

void
CondVarBase::lock () const {
    m_mutex->lock ();
}

void
CondVarBase::unlock () const {
    m_mutex->unlock ();
}

void
CondVarBase::signal () {
    ARCH->signalCondVar (m_cond);
}

void
CondVarBase::broadcast () {
    ARCH->broadcastCondVar (m_cond);
}

bool
CondVarBase::wait (Stopwatch& timer, double timeout) const {
    double remain = timeout - timer.getTime ();
    // Some ARCH wait()s return prematurely, retry until really timed out
    // In particular, ArchMultithreadPosix::waitCondVar() returns every 100ms
    do {
        // Always call wait at least once, even if remain is 0, to give
        // other thread a chance to grab the mutex to avoid deadlocks on
        // busy waiting.
        if (remain < 0.0)
            remain = 0.0;
        if (wait (remain))
            return true;
        remain = timeout - timer.getTime ();
    } while (remain >= 0.0);
    return false;
}

bool
CondVarBase::wait (double timeout) const {
    return ARCH->waitCondVar (m_cond, m_mutex->m_mutex, timeout);
}

Mutex*
CondVarBase::getMutex () const {
    return m_mutex;
}

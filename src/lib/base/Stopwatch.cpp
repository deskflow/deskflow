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

#include "base/Stopwatch.h"
#include "arch/Arch.h"

//
// Stopwatch
//

Stopwatch::Stopwatch (bool triggered)
    : m_mark (0.0), m_triggered (triggered), m_stopped (triggered) {
    if (!triggered) {
        m_mark = ARCH->time ();
    }
}

Stopwatch::~Stopwatch () {
    // do nothing
}

double
Stopwatch::reset () {
    if (m_stopped) {
        const double dt = m_mark;
        m_mark          = 0.0;
        return dt;
    } else {
        const double t  = ARCH->time ();
        const double dt = t - m_mark;
        m_mark          = t;
        return dt;
    }
}

void
Stopwatch::stop () {
    if (m_stopped) {
        return;
    }

    // save the elapsed time
    m_mark    = ARCH->time () - m_mark;
    m_stopped = true;
}

void
Stopwatch::start () {
    m_triggered = false;
    if (!m_stopped) {
        return;
    }

    // set the mark such that it reports the time elapsed at stop()
    m_mark    = ARCH->time () - m_mark;
    m_stopped = false;
}

void
Stopwatch::setTrigger () {
    stop ();
    m_triggered = true;
}

double
Stopwatch::getTime () {
    if (m_triggered) {
        const double dt = m_mark;
        start ();
        return dt;
    } else if (m_stopped) {
        return m_mark;
    } else {
        return ARCH->time () - m_mark;
    }
}

Stopwatch::operator double () {
    return getTime ();
}

bool
Stopwatch::isStopped () const {
    return m_stopped;
}

double
Stopwatch::getTime () const {
    if (m_stopped) {
        return m_mark;
    } else {
        return ARCH->time () - m_mark;
    }
}

Stopwatch::operator double () const {
    return getTime ();
}

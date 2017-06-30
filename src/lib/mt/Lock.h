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

#pragma once

#include "common/common.h"

class Mutex;
class CondVarBase;

//! Mutual exclusion lock utility
/*!
This class locks a mutex or condition variable in the c'tor and unlocks
it in the d'tor.  It's easier and safer than manually locking and
unlocking since unlocking must usually be done no matter how a function
exits (including by unwinding due to an exception).
*/
class Lock {
public:
    //! Lock the mutex \c mutex
    Lock (const Mutex* mutex);
    //! Lock the condition variable \c cv
    Lock (const CondVarBase* cv);
    //! Unlock the mutex or condition variable
    ~Lock ();

private:
    // not implemented
    Lock (const Lock&);
    Lock& operator= (const Lock&);

private:
    const Mutex* m_mutex;
};

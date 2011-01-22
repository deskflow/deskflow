/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CLOCK_H
#define CLOCK_H

#include "common.h"

class CMutex;
class CCondVarBase;

//! Mutual exclusion lock utility
/*!
This class locks a mutex or condition variable in the c'tor and unlocks
it in the d'tor.  It's easier and safer than manually locking and
unlocking since unlocking must usually be done no matter how a function
exits (including by unwinding due to an exception).
*/
class CLock {
public:
	//! Lock the mutex \c mutex
	CLock(const CMutex* mutex);
	//! Lock the condition variable \c cv
	CLock(const CCondVarBase* cv);
	//! Unlock the mutex or condition variable
	~CLock();

private:
	// not implemented
	CLock(const CLock&);
	CLock& operator=(const CLock&);

private:
	const CMutex*		m_mutex;
};

#endif

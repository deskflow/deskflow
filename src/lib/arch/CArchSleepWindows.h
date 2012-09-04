/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CARCHSLEEPWINDOWS_H
#define CARCHSLEEPWINDOWS_H

#include "IArchSleep.h"

#define ARCH_SLEEP CArchSleepWindows

//! Win32 implementation of IArchSleep
class CArchSleepWindows : public IArchSleep {
public:
	CArchSleepWindows();
	virtual ~CArchSleepWindows();

	// IArchSleep overrides
	virtual void		sleep(double timeout);
};

#endif

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

#include "CArchSleepWindows.h"
#include "CArch.h"
#include "CArchMultithreadWindows.h"

//
// CArchSleepWindows
//

CArchSleepWindows::CArchSleepWindows()
{
	// do nothing
}

CArchSleepWindows::~CArchSleepWindows()
{
	// do nothing
}

void
CArchSleepWindows::sleep(double timeout)
{
	ARCH->testCancelThread();
	if (timeout < 0.0) {
		return;
	}

	// get the cancel event from the current thread.  this only
	// works if we're using the windows multithread object but
	// this is windows so that's pretty certain;  we'll get a
	// link error if we're not, though.
	CArchMultithreadWindows* mt = CArchMultithreadWindows::getInstance();
	if (mt != NULL) {
		HANDLE cancelEvent = mt->getCancelEventForCurrentThread();
		WaitForSingleObject(cancelEvent, (DWORD)(1000.0 * timeout));
		if (timeout == 0.0) {
			Sleep(0);
		}
	}
	else {
		Sleep((DWORD)(1000.0 * timeout));
	}
	ARCH->testCancelThread();	
}

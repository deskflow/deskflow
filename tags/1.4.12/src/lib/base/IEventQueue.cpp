/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "IEventQueue.h"
#include "CLog.h"

#if WINAPI_CARBON
#include <execinfo.h>
#include <stdio.h>
#endif


//
// IEventQueue
//

static int				g_systemTarget = 0;
IEventQueue*			IEventQueue::s_instance = NULL;

void*
IEventQueue::getSystemTarget()
{
	// any unique arbitrary pointer will do
	return &g_systemTarget;
}

IEventQueue*
IEventQueue::getInstance()
{
	if (s_instance == NULL) {
		LOG((CLOG_ERR "null event queue"));
#if WINAPI_CARBON
		void* callstack[128];
		int i, frames = backtrace(callstack, 128);
		char** strs = backtrace_symbols(callstack, frames);
		for (i = 0; i < frames; ++i) {
			printf("%s\n", strs[i]);
		}
		free(strs);
#endif
	}
	assert(s_instance != NULL);
	return s_instance;
}

void
IEventQueue::setInstance(IEventQueue* instance)
{
	assert(s_instance == NULL || instance == NULL);
	s_instance = instance;
}

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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
	assert(s_instance != NULL);
	return s_instance;
}

void
IEventQueue::setInstance(IEventQueue* instance)
{
	assert(s_instance == NULL || instance == NULL);
	s_instance = instance;
}

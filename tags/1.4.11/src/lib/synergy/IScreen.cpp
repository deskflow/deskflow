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

#include "IScreen.h"
#include "CEventQueue.h"

//
// IScreen
//

CEvent::Type			IScreen::s_errorEvent            = CEvent::kUnknown;
CEvent::Type			IScreen::s_shapeChangedEvent     = CEvent::kUnknown;
CEvent::Type			IScreen::s_clipboardGrabbedEvent = CEvent::kUnknown;
CEvent::Type			IScreen::s_suspendEvent          = CEvent::kUnknown;
CEvent::Type			IScreen::s_resumeEvent           = CEvent::kUnknown;

CEvent::Type
IScreen::getErrorEvent()
{
	return EVENTQUEUE->registerTypeOnce(s_errorEvent,
							"IScreen::error");
}

CEvent::Type
IScreen::getShapeChangedEvent()
{
	return EVENTQUEUE->registerTypeOnce(s_shapeChangedEvent,
							"IScreen::shapeChanged");
}

CEvent::Type
IScreen::getClipboardGrabbedEvent()
{
	return EVENTQUEUE->registerTypeOnce(s_clipboardGrabbedEvent,
							"IScreen::clipboardGrabbed");
}

CEvent::Type
IScreen::getSuspendEvent()
{
	return EVENTQUEUE->registerTypeOnce(s_suspendEvent,
									"IScreen::suspend");
}

CEvent::Type
IScreen::getResumeEvent()
{
	return EVENTQUEUE->registerTypeOnce(s_resumeEvent,
									"IScreen::resume");
}

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

#include "IKeyState.h"
#include "CEventQueue.h"
#include <cstring>
#include <cstdlib>

//
// IKeyState
//

CEvent::Type		IKeyState::s_keyDownEvent   = CEvent::kUnknown;
CEvent::Type		IKeyState::s_keyUpEvent     = CEvent::kUnknown;
CEvent::Type		IKeyState::s_keyRepeatEvent = CEvent::kUnknown;

IKeyState::IKeyState() :
	m_eventQueue(*EVENTQUEUE)
{
}

IKeyState::IKeyState(IEventQueue& eventQueue) :
	m_eventQueue(eventQueue)
{
}

CEvent::Type
IKeyState::getKeyDownEvent(IEventQueue& eventQueue)
{
	return eventQueue.registerTypeOnce(s_keyDownEvent,
							"IKeyState::keyDown");
}

CEvent::Type
IKeyState::getKeyUpEvent(IEventQueue& eventQueue)
{
	return eventQueue.registerTypeOnce(s_keyUpEvent,
							"IKeyState::keyUp");
}

CEvent::Type
IKeyState::getKeyRepeatEvent(IEventQueue& eventQueue)
{
	return eventQueue.registerTypeOnce(s_keyRepeatEvent,
							"IKeyState::keyRepeat");
}

//
// IKeyState::CKeyInfo
//

IKeyState::CKeyInfo*
IKeyState::CKeyInfo::alloc(KeyID id,
				KeyModifierMask mask, KeyButton button, SInt32 count)
{
	CKeyInfo* info           = (CKeyInfo*)malloc(sizeof(CKeyInfo));
	info->m_key              = id;
	info->m_mask             = mask;
	info->m_button           = button;
	info->m_count            = count;
	info->m_screens          = NULL;
	info->m_screensBuffer[0] = '\0';
	return info;
}

IKeyState::CKeyInfo*
IKeyState::CKeyInfo::alloc(KeyID id,
				KeyModifierMask mask, KeyButton button, SInt32 count,
				const std::set<CString>& destinations)
{
	CString screens = join(destinations);

	// build structure
	CKeyInfo* info  = (CKeyInfo*)malloc(sizeof(CKeyInfo) + screens.size());
	info->m_key     = id;
	info->m_mask    = mask;
	info->m_button  = button;
	info->m_count   = count;
	info->m_screens = info->m_screensBuffer;
	strcpy(info->m_screensBuffer, screens.c_str());
	return info;
}

IKeyState::CKeyInfo*
IKeyState::CKeyInfo::alloc(const CKeyInfo& x)
{
	CKeyInfo* info  = (CKeyInfo*)malloc(sizeof(CKeyInfo) +
										strlen(x.m_screensBuffer));
	info->m_key     = x.m_key;
	info->m_mask    = x.m_mask;
	info->m_button  = x.m_button;
	info->m_count   = x.m_count;
	info->m_screens = x.m_screens ? info->m_screensBuffer : NULL;
	strcpy(info->m_screensBuffer, x.m_screensBuffer);
	return info;
}

bool
IKeyState::CKeyInfo::isDefault(const char* screens)
{
	return (screens == NULL || screens[0] == '\0');
}

bool
IKeyState::CKeyInfo::contains(const char* screens, const CString& name)
{
	// special cases
	if (isDefault(screens)) {
		return false;
	}
	if (screens[0] == '*') {
		return true;
	}

	// search
	CString match;
	match.reserve(name.size() + 2);
	match += ":";
	match += name;
	match += ":";
	return (strstr(screens, match.c_str()) != NULL);
}

bool
IKeyState::CKeyInfo::equal(const CKeyInfo* a, const CKeyInfo* b)
{
	return (a->m_key    == b->m_key &&
			a->m_mask   == b->m_mask &&
			a->m_button == b->m_button &&
			a->m_count  == b->m_count &&
			strcmp(a->m_screensBuffer, b->m_screensBuffer) == 0);
}

CString
IKeyState::CKeyInfo::join(const std::set<CString>& destinations)
{
	// collect destinations into a string.  names are surrounded by ':'
	// which makes searching easy.  the string is empty if there are no
	// destinations and "*" means all destinations.
	CString screens;
	for (std::set<CString>::const_iterator i = destinations.begin();
								i != destinations.end(); ++i) {
		if (*i == "*") {
			screens = "*";
			break;
		}
		else {
			if (screens.empty()) {
				screens = ":";
			}
			screens += *i;
			screens += ":";
		}
	}
	return screens;
}

void
IKeyState::CKeyInfo::split(const char* screens, std::set<CString>& dst)
{
	dst.clear();
	if (isDefault(screens)) {
		return;
	}
	if (screens[0] == '*') {
		dst.insert("*");
		return;
	}

	const char* i = screens + 1;
	while (*i != '\0') {
		const char* j = strchr(i, ':');
		dst.insert(CString(i, j - i));
		i = j + 1;
	}
}

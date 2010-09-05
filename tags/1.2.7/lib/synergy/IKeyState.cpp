/*
 * synergy -- mouse and keyboard sharing utility
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
 */

#include "IKeyState.h"

//
// IKeyState
//

CEvent::Type		IKeyState::s_keyDownEvent   = CEvent::kUnknown;
CEvent::Type		IKeyState::s_keyUpEvent     = CEvent::kUnknown;
CEvent::Type		IKeyState::s_keyRepeatEvent = CEvent::kUnknown;

CEvent::Type
IKeyState::getKeyDownEvent()
{
	return CEvent::registerTypeOnce(s_keyDownEvent,
							"IKeyState::keyDown");
}

CEvent::Type
IKeyState::getKeyUpEvent()
{
	return CEvent::registerTypeOnce(s_keyUpEvent,
							"IKeyState::keyUp");
}

CEvent::Type
IKeyState::getKeyRepeatEvent()
{
	return CEvent::registerTypeOnce(s_keyRepeatEvent,
							"IKeyState::keyRepeat");
}


//
// IKeyState::CKeyInfo
//

IKeyState::CKeyInfo*
IKeyState::CKeyInfo::alloc(KeyID id,
				KeyModifierMask mask, KeyButton button, SInt32 count)
{
	CKeyInfo* info = (CKeyInfo*)malloc(sizeof(CKeyInfo));
	info->m_key    = id;
	info->m_mask   = mask;
	info->m_button = button;
	info->m_count  = count;
	return info;
}

IKeyState::CKeyInfo*
IKeyState::CKeyInfo::alloc(const CKeyInfo& x)
{
	CKeyInfo* info = (CKeyInfo*)malloc(sizeof(CKeyInfo));
	info->m_key    = x.m_key;
	info->m_mask   = x.m_mask;
	info->m_button = x.m_button;
	info->m_count  = x.m_count;
	return info;
}

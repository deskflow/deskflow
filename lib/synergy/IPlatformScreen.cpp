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

#include "IPlatformScreen.h"

//
// IPlatformScreen
//

CEvent::Type		IPlatformScreen::s_keyDownEvent         = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_keyUpEvent           = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_keyRepeatEvent       = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_buttonDownEvent      = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_buttonUpEvent        = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_motionPrimaryEvent   = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_motionSecondaryEvent = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_wheelEvent           = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_ssActivatedEvent     = CEvent::kUnknown;
CEvent::Type		IPlatformScreen::s_ssDeactivatedEvent   = CEvent::kUnknown;

CEvent::Type
IPlatformScreen::getKeyDownEvent()
{
	return CEvent::registerTypeOnce(s_keyDownEvent,
							"IPlatformScreen::keyDown");
}

CEvent::Type
IPlatformScreen::getKeyUpEvent()
{
	return CEvent::registerTypeOnce(s_keyUpEvent,
							"IPlatformScreen::keyUp");
}

CEvent::Type
IPlatformScreen::getKeyRepeatEvent()
{
	return CEvent::registerTypeOnce(s_keyRepeatEvent,
							"IPlatformScreen::keyRepeat");
}

CEvent::Type
IPlatformScreen::getButtonDownEvent()
{
	return CEvent::registerTypeOnce(s_buttonDownEvent,
							"IPlatformScreen::buttonDown");
}

CEvent::Type
IPlatformScreen::getButtonUpEvent()
{
	return CEvent::registerTypeOnce(s_buttonUpEvent,
							"IPlatformScreen::buttonUp");
}

CEvent::Type
IPlatformScreen::getMotionOnPrimaryEvent()
{
	return CEvent::registerTypeOnce(s_motionPrimaryEvent,
							"IPlatformScreen::motionPrimary");
}

CEvent::Type
IPlatformScreen::getMotionOnSecondaryEvent()
{
	return CEvent::registerTypeOnce(s_motionSecondaryEvent,
							"IPlatformScreen::motionSecondary");
}

CEvent::Type
IPlatformScreen::getWheelEvent()
{
	return CEvent::registerTypeOnce(s_wheelEvent,
							"IPlatformScreen::wheel");
}

CEvent::Type
IPlatformScreen::getScreensaverActivatedEvent()
{
	return CEvent::registerTypeOnce(s_ssActivatedEvent,
							"IPlatformScreen::screensaverActivated");
}

CEvent::Type
IPlatformScreen::getScreensaverDeactivatedEvent()
{
	return CEvent::registerTypeOnce(s_ssDeactivatedEvent,
							"IPlatformScreen::screensaverDeactivated");
}


//
// IPlatformScreen::CKeyInfo
//

IPlatformScreen::CKeyInfo*
IPlatformScreen::CKeyInfo::alloc(KeyID id,
				KeyModifierMask mask, KeyButton button, SInt32 count)
{
	CKeyInfo* info = (CKeyInfo*)malloc(sizeof(CKeyInfo));
	info->m_key    = id;
	info->m_mask   = mask;
	info->m_button = button;
	info->m_count  = count;
	return info;
}


//
// IPlatformScreen::CButtonInfo
//

IPlatformScreen::CButtonInfo*
IPlatformScreen::CButtonInfo::alloc(ButtonID id)
{
	CButtonInfo* info = (CButtonInfo*)malloc(sizeof(CButtonInfo));
	info->m_button = id;
	return info;
}


//
// IPlatformScreen::CMotionInfo
//

IPlatformScreen::CMotionInfo*
IPlatformScreen::CMotionInfo::alloc(SInt32 x, SInt32 y)
{
	CMotionInfo* info = (CMotionInfo*)malloc(sizeof(CMotionInfo));
	info->m_x = x;
	info->m_y = y;
	return info;
}


//
// IPlatformScreen::CWheelInfo
//

IPlatformScreen::CWheelInfo*
IPlatformScreen::CWheelInfo::alloc(SInt32 wheel)
{
	CWheelInfo* info = (CWheelInfo*)malloc(sizeof(CWheelInfo));
	info->m_wheel = wheel;
	return info;
}

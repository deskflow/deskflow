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

#include "IPrimaryScreen.h"

//
// IPrimaryScreen
//

CEvent::Type		IPrimaryScreen::s_buttonDownEvent      = CEvent::kUnknown;
CEvent::Type		IPrimaryScreen::s_buttonUpEvent        = CEvent::kUnknown;
CEvent::Type		IPrimaryScreen::s_motionPrimaryEvent   = CEvent::kUnknown;
CEvent::Type		IPrimaryScreen::s_motionSecondaryEvent = CEvent::kUnknown;
CEvent::Type		IPrimaryScreen::s_wheelEvent           = CEvent::kUnknown;
CEvent::Type		IPrimaryScreen::s_ssActivatedEvent     = CEvent::kUnknown;
CEvent::Type		IPrimaryScreen::s_ssDeactivatedEvent   = CEvent::kUnknown;
CEvent::Type		IPrimaryScreen::s_hotKeyDownEvent      = CEvent::kUnknown;
CEvent::Type		IPrimaryScreen::s_hotKeyUpEvent        = CEvent::kUnknown;

CEvent::Type
IPrimaryScreen::getButtonDownEvent()
{
	return CEvent::registerTypeOnce(s_buttonDownEvent,
							"IPrimaryScreen::buttonDown");
}

CEvent::Type
IPrimaryScreen::getButtonUpEvent()
{
	return CEvent::registerTypeOnce(s_buttonUpEvent,
							"IPrimaryScreen::buttonUp");
}

CEvent::Type
IPrimaryScreen::getMotionOnPrimaryEvent()
{
	return CEvent::registerTypeOnce(s_motionPrimaryEvent,
							"IPrimaryScreen::motionPrimary");
}

CEvent::Type
IPrimaryScreen::getMotionOnSecondaryEvent()
{
	return CEvent::registerTypeOnce(s_motionSecondaryEvent,
							"IPrimaryScreen::motionSecondary");
}

CEvent::Type
IPrimaryScreen::getWheelEvent()
{
	return CEvent::registerTypeOnce(s_wheelEvent,
							"IPrimaryScreen::wheel");
}

CEvent::Type
IPrimaryScreen::getScreensaverActivatedEvent()
{
	return CEvent::registerTypeOnce(s_ssActivatedEvent,
							"IPrimaryScreen::screensaverActivated");
}

CEvent::Type
IPrimaryScreen::getScreensaverDeactivatedEvent()
{
	return CEvent::registerTypeOnce(s_ssDeactivatedEvent,
							"IPrimaryScreen::screensaverDeactivated");
}

CEvent::Type
IPrimaryScreen::getHotKeyDownEvent()
{
	return CEvent::registerTypeOnce(s_hotKeyDownEvent,
							"IPrimaryScreen::hotKeyDown");
}

CEvent::Type
IPrimaryScreen::getHotKeyUpEvent()
{
	return CEvent::registerTypeOnce(s_hotKeyUpEvent,
							"IPrimaryScreen::hotKeyUp");
}


//
// IPrimaryScreen::CButtonInfo
//

IPrimaryScreen::CButtonInfo*
IPrimaryScreen::CButtonInfo::alloc(ButtonID id, KeyModifierMask mask)
{
	CButtonInfo* info = (CButtonInfo*)malloc(sizeof(CButtonInfo));
	info->m_button = id;
	info->m_mask   = mask;
	return info;
}

IPrimaryScreen::CButtonInfo*
IPrimaryScreen::CButtonInfo::alloc(const CButtonInfo& x)
{
	CButtonInfo* info = (CButtonInfo*)malloc(sizeof(CButtonInfo));
	info->m_button = x.m_button;
	info->m_mask   = x.m_mask;
	return info;
}


//
// IPrimaryScreen::CMotionInfo
//

IPrimaryScreen::CMotionInfo*
IPrimaryScreen::CMotionInfo::alloc(SInt32 x, SInt32 y)
{
	CMotionInfo* info = (CMotionInfo*)malloc(sizeof(CMotionInfo));
	info->m_x = x;
	info->m_y = y;
	return info;
}


//
// IPrimaryScreen::CWheelInfo
//

IPrimaryScreen::CWheelInfo*
IPrimaryScreen::CWheelInfo::alloc(SInt32 wheel)
{
	CWheelInfo* info = (CWheelInfo*)malloc(sizeof(CWheelInfo));
	info->m_wheel = wheel;
	return info;
}


//
// IPrimaryScreen::CHotKeyInfo
//

IPrimaryScreen::CHotKeyInfo*
IPrimaryScreen::CHotKeyInfo::alloc(UInt32 id)
{
	CHotKeyInfo* info = (CHotKeyInfo*)malloc(sizeof(CHotKeyInfo));
	info->m_id = id;
	return info;
}

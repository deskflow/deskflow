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

#include "IPrimaryScreen.h"
#include "CEventQueue.h"
#include <cstdlib>

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

bool
IPrimaryScreen::CButtonInfo::equal(const CButtonInfo* a, const CButtonInfo* b)
{
	return (a->m_button == b->m_button && a->m_mask == b->m_mask);
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
IPrimaryScreen::CWheelInfo::alloc(SInt32 xDelta, SInt32 yDelta)
{
	CWheelInfo* info = (CWheelInfo*)malloc(sizeof(CWheelInfo));
	info->m_xDelta = xDelta;
	info->m_yDelta = yDelta;
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

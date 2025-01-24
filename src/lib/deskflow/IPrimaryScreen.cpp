/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/IPrimaryScreen.h"
#include "base/EventQueue.h"

#include <cstdlib>

//
// IPrimaryScreen::ButtonInfo
//

IPrimaryScreen::ButtonInfo *IPrimaryScreen::ButtonInfo::alloc(ButtonID id, KeyModifierMask mask)
{
  ButtonInfo *info = (ButtonInfo *)malloc(sizeof(ButtonInfo));
  info->m_button = id;
  info->m_mask = mask;
  return info;
}

IPrimaryScreen::ButtonInfo *IPrimaryScreen::ButtonInfo::alloc(const ButtonInfo &x)
{
  ButtonInfo *info = (ButtonInfo *)malloc(sizeof(ButtonInfo));
  info->m_button = x.m_button;
  info->m_mask = x.m_mask;
  return info;
}

bool IPrimaryScreen::ButtonInfo::equal(const ButtonInfo *a, const ButtonInfo *b)
{
  return (a->m_button == b->m_button && a->m_mask == b->m_mask);
}

//
// IPrimaryScreen::MotionInfo
//

IPrimaryScreen::MotionInfo *IPrimaryScreen::MotionInfo::alloc(int32_t x, int32_t y)
{
  MotionInfo *info = (MotionInfo *)malloc(sizeof(MotionInfo));
  info->m_x = x;
  info->m_y = y;
  return info;
}

//
// IPrimaryScreen::WheelInfo
//

IPrimaryScreen::WheelInfo *IPrimaryScreen::WheelInfo::alloc(int32_t xDelta, int32_t yDelta)
{
  WheelInfo *info = (WheelInfo *)malloc(sizeof(WheelInfo));
  info->m_xDelta = xDelta;
  info->m_yDelta = yDelta;
  return info;
}

//
// IPrimaryScreen::HotKeyInfo
//

IPrimaryScreen::HotKeyInfo *IPrimaryScreen::HotKeyInfo::alloc(uint32_t id)
{
  HotKeyInfo *info = (HotKeyInfo *)malloc(sizeof(HotKeyInfo));
  info->m_id = id;
  return info;
}

//
// IPrimaryScreen::EiConnectInfo
//

IPrimaryScreen::EiConnectInfo *IPrimaryScreen::EiConnectInfo::alloc(int fd)
{
  EiConnectInfo *info = (EiConnectInfo *)malloc(sizeof(EiConnectInfo));
  info->m_fd = fd;
  return info;
}

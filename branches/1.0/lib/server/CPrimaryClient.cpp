/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
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

#include "CPrimaryClient.h"
#include "IPrimaryScreenFactory.h"
#include "IServer.h"
#include "XScreen.h"
#include "XSynergy.h"
#include "CPrimaryScreen.h"
#include "CClipboard.h"
#include "CLog.h"

//
// CPrimaryClient
//

CPrimaryClient::CPrimaryClient(IPrimaryScreenFactory* screenFactory,
				IServer* server,
				IPrimaryScreenReceiver* receiver,
				const CString& name) :
	m_server(server),
	m_name(name),
	m_seqNum(0)
{
	assert(m_server != NULL);

	// create screen
	LOG((CLOG_DEBUG1 "creating primary screen"));
	if (screenFactory != NULL) {
		m_screen = screenFactory->create(this, receiver);
	}
	if (m_screen == NULL) {
		throw XScreenOpenFailure();
	}
}

CPrimaryClient::~CPrimaryClient()
{
	LOG((CLOG_DEBUG1 "destroying primary screen"));
	delete m_screen;
}

void
CPrimaryClient::exitMainLoop()
{
	m_screen->exitMainLoop();
}

void
CPrimaryClient::reconfigure(UInt32 activeSides)
{
	m_screen->reconfigure(activeSides);
}

UInt32
CPrimaryClient::addOneShotTimer(double timeout)
{
	return m_screen->addOneShotTimer(timeout);
}

void
CPrimaryClient::getClipboard(ClipboardID id, CString& data) const
{
	CClipboard clipboard;
	m_screen->getClipboard(id, &clipboard);
	data = clipboard.marshall();
}

bool
CPrimaryClient::isLockedToScreen() const
{
	return m_screen->isLockedToScreen();
}

KeyModifierMask
CPrimaryClient::getToggleMask() const
{
	return m_screen->getToggleMask();
}

void
CPrimaryClient::onError()
{
	// forward to server
	m_server->onError();
}

void
CPrimaryClient::onInfoChanged(const CClientInfo& info)
{
	m_info = info;
	try {
		m_server->onInfoChanged(getName(), m_info);
	}
	catch (XBadClient&) {
		// ignore
	}
}

bool
CPrimaryClient::onGrabClipboard(ClipboardID id)
{
	try {
		return m_server->onGrabClipboard(getName(), id, m_seqNum);
	}
	catch (XBadClient&) {
		return false;
	}
}

void
CPrimaryClient::onClipboardChanged(ClipboardID id, const CString& data)
{
	m_server->onClipboardChanged(id, m_seqNum, data);
}

void
CPrimaryClient::open()
{
	// all clipboards are clean
	for (UInt32 i = 0; i < kClipboardEnd; ++i) {
		m_clipboardDirty[i] = false;
	}

	// now open the screen
	m_screen->open();
}

void
CPrimaryClient::mainLoop()
{
	m_screen->mainLoop();
}

void
CPrimaryClient::close()
{
	m_screen->close();
}

void
CPrimaryClient::enter(SInt32 xAbs, SInt32 yAbs,
				UInt32 seqNum, KeyModifierMask, bool screensaver)
{
	// note -- we must not call any server methods except onError().
	m_seqNum = seqNum;
	m_screen->enter(xAbs, yAbs, screensaver);
}

bool
CPrimaryClient::leave()
{
	// note -- we must not call any server methods except onError().
	return m_screen->leave();
}

void
CPrimaryClient::setClipboard(ClipboardID id, const CString& data)
{
	// note -- we must not call any server methods except onError().

	// ignore if this clipboard is already clean
	if (m_clipboardDirty[id]) {
		// this clipboard is now clean
		m_clipboardDirty[id] = false;

		// unmarshall data
		CClipboard clipboard;
		clipboard.unmarshall(data, 0);

		// set clipboard
		m_screen->setClipboard(id, &clipboard);
	}
}

void
CPrimaryClient::grabClipboard(ClipboardID id)
{
	// grab clipboard
	m_screen->grabClipboard(id);

	// clipboard is dirty (because someone else owns it now)
	m_clipboardDirty[id] = true;
}

void
CPrimaryClient::setClipboardDirty(ClipboardID id, bool dirty)
{
	m_clipboardDirty[id] = dirty;
}

void
CPrimaryClient::keyDown(KeyID, KeyModifierMask, KeyButton)
{
	// ignore
}

void
CPrimaryClient::keyRepeat(KeyID, KeyModifierMask, SInt32, KeyButton)
{
	// ignore
}

void
CPrimaryClient::keyUp(KeyID, KeyModifierMask, KeyButton)
{
	// ignore
}

void
CPrimaryClient::mouseDown(ButtonID)
{
	// ignore
}

void
CPrimaryClient::mouseUp(ButtonID)
{
	// ignore
}

void
CPrimaryClient::mouseMove(SInt32 x, SInt32 y)
{
	m_screen->warpCursor(x, y);
}

void
CPrimaryClient::mouseWheel(SInt32)
{
	// ignore
}

void
CPrimaryClient::screensaver(bool)
{
	// ignore
}

void
CPrimaryClient::resetOptions()
{
	m_screen->resetOptions();
}

void
CPrimaryClient::setOptions(const COptionsList& options)
{
	m_screen->setOptions(options);
}

CString
CPrimaryClient::getName() const
{
	return m_name;
}

SInt32
CPrimaryClient::getJumpZoneSize() const
{
	return m_info.m_zoneSize;
}

void
CPrimaryClient::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	x = m_info.m_x;
	y = m_info.m_y;
	w = m_info.m_w;
	h = m_info.m_h;
}

void
CPrimaryClient::getCursorPos(SInt32&, SInt32&) const
{
	assert(0 && "shouldn't be called");
}

void
CPrimaryClient::getCursorCenter(SInt32& x, SInt32& y) const
{
	x = m_info.m_mx;
	y = m_info.m_my;
}

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#include "CClientTaskBarReceiver.h"
#include "CClient.h"
#include "CLock.h"
#include "IEventQueue.h"
#include "CArch.h"

//
// CClientTaskBarReceiver
//

CClientTaskBarReceiver::CClientTaskBarReceiver() :
	m_state(kNotRunning)
{
	// do nothing
}

CClientTaskBarReceiver::~CClientTaskBarReceiver()
{
	// do nothing
}

void
CClientTaskBarReceiver::updateStatus(CClient* client, const CString& errorMsg)
{
	{
		// update our status
		m_errorMessage = errorMsg;
		if (client == NULL) {
			if (m_errorMessage.empty()) {
				m_state = kNotRunning;
			}
			else {
				m_state = kNotWorking;
			}
		}
		else {
			if (client->isConnected()) {
				m_state = kConnected;
			}
			else if (client->isConnecting()) {
				m_state = kConnecting;
			}
			else {
				m_state = kNotConnected;
			}
		}

		// let subclasses have a go
		onStatusChanged(client);
	}

	// tell task bar
	ARCH->updateReceiver(this);
}

CClientTaskBarReceiver::EState
CClientTaskBarReceiver::getStatus() const
{
	return m_state;
}

const CString&
CClientTaskBarReceiver::getErrorMessage() const
{
	return m_errorMessage;
}

void
CClientTaskBarReceiver::quit()
{
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}

void
CClientTaskBarReceiver::onStatusChanged(CClient*)
{
	// do nothing
}

void
CClientTaskBarReceiver::lock() const
{
	// do nothing
}

void
CClientTaskBarReceiver::unlock() const
{
	// do nothing
}

std::string
CClientTaskBarReceiver::getToolTip() const
{
	switch (m_state) {
	case kNotRunning:
		return "Synergy:  Not running";

	case kNotWorking:
		return CString("Synergy:  ") + m_errorMessage;

	case kNotConnected:
		return CString("Synergy:  Not connected:  ") + m_errorMessage;

	case kConnecting:
		return "Synergy:  Connecting...";

	case kConnected:
		return "Synergy:  Connected";

	default:
		return "";
	}
}

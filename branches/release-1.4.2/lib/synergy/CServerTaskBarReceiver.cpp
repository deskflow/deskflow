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

#include "CServerTaskBarReceiver.h"
#include "CServer.h"
#include "CLock.h"
#include "CStringUtil.h"
#include "IEventQueue.h"
#include "CArch.h"
#include "Version.h"

//
// CServerTaskBarReceiver
//

CServerTaskBarReceiver::CServerTaskBarReceiver() :
	m_state(kNotRunning)
{
	// do nothing
}

CServerTaskBarReceiver::~CServerTaskBarReceiver()
{
	// do nothing
}

void
CServerTaskBarReceiver::updateStatus(CServer* server, const CString& errorMsg)
{
	{
		// update our status
		m_errorMessage = errorMsg;
		if (server == NULL) {
			if (m_errorMessage.empty()) {
				m_state = kNotRunning;
			}
			else {
				m_state = kNotWorking;
			}
		}
		else {
			m_clients.clear();
			server->getClients(m_clients);
			if (m_clients.size() <= 1) {
				m_state = kNotConnected;
			}
			else {
				m_state = kConnected;
			}
		}

		// let subclasses have a go
		onStatusChanged(server);
	}

	// tell task bar
	ARCH->updateReceiver(this);
}

CServerTaskBarReceiver::EState
CServerTaskBarReceiver::getStatus() const
{
	return m_state;
}

const CString&
CServerTaskBarReceiver::getErrorMessage() const
{
	return m_errorMessage;
}

const CServerTaskBarReceiver::CClients&
CServerTaskBarReceiver::getClients() const
{
	return m_clients;
}

void
CServerTaskBarReceiver::quit()
{
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}

void
CServerTaskBarReceiver::onStatusChanged(CServer*)
{
	// do nothing
}

void
CServerTaskBarReceiver::lock() const
{
	// do nothing
}

void
CServerTaskBarReceiver::unlock() const
{
	// do nothing
}

std::string
CServerTaskBarReceiver::getToolTip() const
{
	switch (m_state) {
	case kNotRunning:
		return CStringUtil::print("%s:  Not running", kAppVersion);

	case kNotWorking:
		return CStringUtil::print("%s:  %s",
								kAppVersion, m_errorMessage.c_str());
						
	case kNotConnected:
		return CStringUtil::print("%s:  Waiting for clients", kAppVersion);

	case kConnected:
		return CStringUtil::print("%s:  Connected", kAppVersion);

	default:
		return "";
	}
}

CEvent::Type
CServerTaskBarReceiver::getReloadConfigEvent()
{
	return CServerApp::instance().getReloadConfigEvent();
}

CEvent::Type
CServerTaskBarReceiver::getForceReconnectEvent()
{
	return CServerApp::instance().getForceReconnectEvent();
}

CEvent::Type
CServerTaskBarReceiver::getResetServerEvent()
{
	return CServerApp::instance().getResetServerEvent();
}

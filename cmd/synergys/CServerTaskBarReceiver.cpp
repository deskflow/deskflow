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
#include "TMethodJob.h"
#include "CArch.h"

//
// CServerTaskBarReceiver
//

CServerTaskBarReceiver::CServerTaskBarReceiver() :
	m_quit(NULL),
	m_state(kNotRunning),
	m_server(NULL)
{
	// create a job for getting notification when the server's
	// status changes.
	m_job = new TMethodJob<CServerTaskBarReceiver>(this,
							&CServerTaskBarReceiver::statusChanged, NULL);
}

CServerTaskBarReceiver::~CServerTaskBarReceiver()
{
	if (m_server != NULL) {
		m_server->removeStatusJob(m_job);
	}
	delete m_job;
	delete m_quit;
}

void
CServerTaskBarReceiver::setServer(CServer* server)
{
	{
		CLock lock(&m_mutex);
		if (m_server != server) {
			if (m_server != NULL) {
				m_server->removeStatusJob(m_job);
			}
			m_server = server;
			if (m_server != NULL) {
				m_server->addStatusJob(m_job);
			}
		}
	}
	ARCH->updateReceiver(this);
}

void
CServerTaskBarReceiver::setState(EState state)
{
	{
		CLock lock(&m_mutex);
		m_state = state;
	}
	ARCH->updateReceiver(this);
}

void
CServerTaskBarReceiver::setQuitJob(IJob* job)
{
	CLock lock(&m_mutex);
	delete m_quit;
	m_quit = job;
}

CServerTaskBarReceiver::EState
CServerTaskBarReceiver::getState() const
{
	return m_state;
}

CServer*
CServerTaskBarReceiver::getServer() const
{
	return m_server;
}

void
CServerTaskBarReceiver::lock() const
{
	m_mutex.lock();
}

void
CServerTaskBarReceiver::unlock() const
{
	m_mutex.unlock();
}

std::string
CServerTaskBarReceiver::getToolTip() const
{
	switch (m_state) {
	case kNotRunning:
		return "Synergy:  Not running";

	case kNotWorking:
		return CString("Synergy:  ") + m_errorMessage;

	case kNotConnected:
		return "Synergy:  Waiting for clients";

	case kConnected:
		return "Synergy:  Connected";

	default:
		return "";
	}
}

void
CServerTaskBarReceiver::quit()
{
	if (m_quit != NULL) {
		m_quit->run();
	}
}

void
CServerTaskBarReceiver::onStatusChanged()
{
	// do nothing
}

void
CServerTaskBarReceiver::statusChanged(void*)
{
	// update our status
	switch (m_server->getStatus(&m_errorMessage)) {
	case CServer::kNotRunning:
		setState(kNotRunning);
		break;

	case CServer::kRunning:
		if (m_server->getNumClients() > 1)
			setState(kConnected);
		else
			setState(kNotConnected);
		break;

	case CServer::kServerNameUnknown:
		m_errorMessage = "Server name is not in configuration";
		setState(kNotWorking);
		break;

	case CServer::kError:
		setState(kNotWorking);
		break;

	default:
		break;
	}

	// let subclasses have a go
	onStatusChanged();
}

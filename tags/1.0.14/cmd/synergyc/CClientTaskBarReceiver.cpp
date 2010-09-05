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
#include "TMethodJob.h"
#include "CArch.h"

//
// CClientTaskBarReceiver
//

CClientTaskBarReceiver::CClientTaskBarReceiver() :
	m_quit(NULL),
	m_state(kNotRunning),
	m_client(NULL)
{
	// create a job for getting notification when the client's
	// status changes.
	m_job = new TMethodJob<CClientTaskBarReceiver>(this,
							&CClientTaskBarReceiver::statusChanged, NULL);
}

CClientTaskBarReceiver::~CClientTaskBarReceiver()
{
	if (m_client != NULL) {
		m_client->removeStatusJob(m_job);
	}
	delete m_job;
	delete m_quit;
}

void
CClientTaskBarReceiver::setClient(CClient* client)
{
	{
		CLock lock(&m_mutex);
		if (m_client != client) {
			if (m_client != NULL) {
				m_client->removeStatusJob(m_job);
			}
			m_client = client;
			if (m_client != NULL) {
				m_client->addStatusJob(m_job);
			}
		}
	}
	ARCH->updateReceiver(this);
}

void
CClientTaskBarReceiver::setState(EState state)
{
	{
		CLock lock(&m_mutex);
		m_state = state;
	}
	ARCH->updateReceiver(this);
}

void
CClientTaskBarReceiver::setQuitJob(IJob* job)
{
	CLock lock(&m_mutex);
	delete m_quit;
	m_quit = job;
}

CClientTaskBarReceiver::EState
CClientTaskBarReceiver::getState() const
{
	return m_state;
}

CClient*
CClientTaskBarReceiver::getClient() const
{
	return m_client;
}

void
CClientTaskBarReceiver::lock() const
{
	m_mutex.lock();
}

void
CClientTaskBarReceiver::unlock() const
{
	m_mutex.unlock();
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
		return "Synergy:  Waiting for clients";

	case kConnected:
		return "Synergy:  Connected";

	default:
		return "";
	}
}

void
CClientTaskBarReceiver::quit()
{
	if (m_quit != NULL) {
		m_quit->run();
	}
}

void
CClientTaskBarReceiver::onStatusChanged()
{
	// do nothing
}

void
CClientTaskBarReceiver::statusChanged(void*)
{
	// update our status
	switch (m_client->getStatus(&m_errorMessage)) {
	case CClient::kNotRunning:
		setState(kNotRunning);
		break;

	case CClient::kRunning:
		setState(kConnected);
		break;

	case CClient::kError:
		setState(kNotWorking);
		break;

	default:
		break;
	}

	// let subclasses have a go
	onStatusChanged();
}

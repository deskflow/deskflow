/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "CString.h"
#include "IArchTaskBarReceiver.h"
#include "stdvector.h"
#include "CEvent.h"
#include "INode.h"
#include "LogOutputters.h"

//! Implementation of IArchTaskBarReceiver for the synergy server
class CPortableTaskBarReceiver : public IArchTaskBarReceiver {
public:
	CPortableTaskBarReceiver();
	virtual ~CPortableTaskBarReceiver();

	//! @name manipulators
	//@{

	//! Update status
	/*!
	Determine the status and query required information from the server.
	*/
	void				updateStatus(INode*, const CString& errorMsg);

	//@}

	// IArchTaskBarReceiver overrides
	virtual void		showStatus() = 0;
	virtual void		runMenu(int x, int y) = 0;
	virtual void		primaryAction() = 0;
	virtual void		lock() const;
	virtual void		unlock() const;
	virtual const Icon	getIcon() const = 0;
	virtual std::string	getToolTip() const;

protected:
	typedef std::vector<CString> CClients;
	enum EState {
		kNotRunning,
		kNotWorking,
		kNotConnected,
		kConnected,
		kMaxState
	};

	//! Get status
	EState				getStatus() const;

	//! Get error message
	const CString&		getErrorMessage() const;

	//! Quit app
	/*!
	Causes the application to quit gracefully
	*/
	void				quit();

	//! Status change notification
	/*!
	Called when status changes.  The default implementation does
	nothing.
	*/
	virtual void		onStatusChanged(INode* node);

protected:
	CEvent::Type getReloadConfigEvent();
	CEvent::Type getForceReconnectEvent();
	CEvent::Type getResetServerEvent();

private:
	EState				m_state;
	CString				m_errorMessage;

	CString				m_server;
	CClients			m_clients;
};

IArchTaskBarReceiver* createTaskBarReceiver(const CBufferedLogOutputter* logBuffer);

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

#ifndef CCLIENTTASKBARRECEIVER_H
#define CCLIENTTASKBARRECEIVER_H

#include "CString.h"
#include "IArchTaskBarReceiver.h"
#include "LogOutputters.h"
#include "CClient.h"

class IEventQueue;

//! Implementation of IArchTaskBarReceiver for the synergy server
class CClientTaskBarReceiver : public IArchTaskBarReceiver {
public:
	CClientTaskBarReceiver(IEventQueue* events);
	virtual ~CClientTaskBarReceiver();

	//! @name manipulators
	//@{

	//! Update status
	/*!
	Determine the status and query required information from the client.
	*/
	void				updateStatus(CClient*, const CString& errorMsg);

	void updateStatus(INode* n, const CString& errorMsg) { updateStatus((CClient*)n, errorMsg); }

	//@}

	// IArchTaskBarReceiver overrides
	virtual void		showStatus() = 0;
	virtual void		runMenu(int x, int y) = 0;
	virtual void		primaryAction() = 0;
	virtual void		lock() const;
	virtual void		unlock() const;
	virtual const Icon	getIcon() const = 0;
	virtual std::string	getToolTip() const;
	virtual void cleanup() {}

protected:
	enum EState {
		kNotRunning,
		kNotWorking,
		kNotConnected,
		kConnecting,
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
	Called when status changes.  The default implementation does nothing.
	*/
	virtual void		onStatusChanged(CClient* client);

private:
	EState				m_state;
	CString				m_errorMessage;
	CString				m_server;
	IEventQueue*		m_events;
};

IArchTaskBarReceiver* createTaskBarReceiver(const CBufferedLogOutputter* logBuffer, IEventQueue* events);

#endif

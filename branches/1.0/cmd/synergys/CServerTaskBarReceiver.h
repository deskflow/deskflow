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

#ifndef CSERVERTASKBARRECEIVER_H
#define CSERVERTASKBARRECEIVER_H

#include "CMutex.h"
#include "CString.h"
#include "IArchTaskBarReceiver.h"

class CServer;
class IJob;

//! Implementation of IArchTaskBarReceiver for the synergy server
class CServerTaskBarReceiver : public IArchTaskBarReceiver {
public:
	enum EState {
		kNotRunning,
		kNotWorking,
		kNotConnected,
		kConnected,
		kMaxState
	};

	CServerTaskBarReceiver();
	virtual ~CServerTaskBarReceiver();

	//! @name manipulators
	//@{

	//! Set server
	/*!
	Sets the server.  The receiver will query state from this server.
	*/
	void				setServer(CServer*);

	//! Set state
	/*!
	Sets the current server state.
	*/
	void				setState(EState);

	//! Set the quit job that causes the server to quit
	/*!
	Set the job that causes the server to quit.
	*/
	void				setQuitJob(IJob* adopted);

	//@}
	//! @name accessors
	//@{

	//! Get state
	/*!
	Returns the current server state.  The receiver is not locked
	by this call;  the caller must do the locking.
	*/
	EState				getState() const;

	//! Get server
	/*!
	Returns the server set by \c setServer().
	*/
	CServer*			getServer() const;

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
	void				quit();

	//! Status change notification
	/*!
	Called when status changes.  The default implementation does
	nothing.
	*/
	virtual void		onStatusChanged();

private:
	void				statusChanged(void*);

private:
	CMutex				m_mutex;
	IJob*				m_quit;
	EState				m_state;
	CServer*			m_server;
	IJob*				m_job;
	CString				m_errorMessage;
};

#endif

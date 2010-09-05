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

#ifndef CCLIENTTASKBARRECEIVER_H
#define CCLIENTTASKBARRECEIVER_H

#include "CMutex.h"
#include "CString.h"
#include "IArchTaskBarReceiver.h"

class CClient;
class IJob;

//! Implementation of IArchTaskBarReceiver for the synergy server
class CClientTaskBarReceiver : public IArchTaskBarReceiver {
public:
	enum EState {
		kNotRunning,
		kNotWorking,
		kNotConnected,
		kConnected,
		kMaxState
	};

	CClientTaskBarReceiver();
	virtual ~CClientTaskBarReceiver();

	//! @name manipulators
	//@{

	//! Set server
	/*!
	Sets the server.  The receiver will query state from this server.
	*/
	void				setClient(CClient*);

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
	Returns the server set by \c setClient().
	*/
	CClient*			getClient() const;

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
	CClient*			m_client;
	IJob*				m_job;
	CString				m_errorMessage;
};

#endif

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

#ifndef CAUTOSTART_H
#define CAUTOSTART_H

#include "CString.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

//! Auto start dialog for Microsoft Windows launcher
class CAutoStart {
public:
	CAutoStart(HWND parent, bool isServer, const CString& cmdLine);
	~CAutoStart();

	//! @name manipulators
	//@{

	//! Run dialog
	/*!
	Display and handle the dialog until closed by the user.
	*/
	void				doModal();

	//! Reinstall daemon
	/*!
	Reinstalls the currently installed daemon.
	*/
	static void			reinstallDaemon(bool isClient, const CString& cmdLine);

	//! Uninstalls daemon
	/*!
	Uninstalls all installed client (\p client is \c true) or server daemons.
	*/
	static void			uninstallDaemons(bool client);

	//! Starts an installed daemon
	/*!
	Returns \c true iff a daemon was started.  This will only start daemons
	installed for all users.
	*/
	static bool			startDaemon();

	//@}
	//! @name accessors
	//@{

	//! Tests if any daemons are installed
	/*!
	Returns \c true if any daemons are installed.
	*/
	static bool			isDaemonInstalled();

	//@}

private:
	void				update();
	bool				onInstall(bool allUsers);
	bool				onUninstall(bool allUsers);

	// message handling
	BOOL				doDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);

private:
	static CAutoStart*	s_singleton;

	HWND				m_parent;
	bool				m_isServer;
	CString				m_cmdLine;
	CString				m_name;
	HWND				m_hwnd;
	bool				m_install;
	CString				m_errorMessage;
};

#endif

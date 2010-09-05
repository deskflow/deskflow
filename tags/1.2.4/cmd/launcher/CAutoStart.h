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

class CConfig;

//! Auto start dialog for Microsoft Windows launcher
class CAutoStart {
public:
	// if config == NULL then it's assumed we're installing/uninstalling
	// the client, otherwise the server.
	CAutoStart(HWND parent, CConfig* config, const CString& cmdLine);
	~CAutoStart();

	//! @name manipulators
	//@{

	//! Run dialog
	/*!
	Display and handle the dialog until closed by the user.
	*/
	void				doModal();

	//@}
	//! @name accessors
	//@{

	//! Test if user configuration was saved
	/*!
	Returns true if the user's configuration (as opposed to the system-wide
	configuration) was saved successfully while in doModal().
	*/
	bool				wasUserConfigSaved() const;

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
	CConfig*			m_config;
	bool				m_isServer;
	CString				m_cmdLine;
	CString				m_name;
	HWND				m_hwnd;
	bool				m_install;
	CString				m_errorMessage;
	bool				m_userConfigSaved;
};

#endif

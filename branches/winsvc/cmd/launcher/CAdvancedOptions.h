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

#ifndef CADVANCEDOPTIONS_H
#define CADVANCEDOPTIONS_H

#include "CString.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

class CConfig;

//! Advanced options dialog for Microsoft Windows launcher
class CAdvancedOptions {
public:
	CAdvancedOptions(HWND parent, CConfig*);
	~CAdvancedOptions();

	//! @name manipulators
	//@{

	//! Run dialog
	/*!
	Display and handle the dialog until closed by the user.
	*/
	void				doModal(bool isClient);

	//@}
	//! @name accessors
	//@{

	//! Get the screen name
	CString				getScreenName() const;

	//! Get the port
	int					getPort() const;

	//! Get the interface
	CString				getInterface() const;

	//! Convert options to command line string
	CString				getCommandLine(bool isClient,
							const CString& serverName) const;

	//@}

private:
	void				init();
	void				doInit(HWND hwnd);
	bool				save(HWND hwnd);
	void				setDefaults(HWND hwnd);

	// message handling
	BOOL				doDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);

private:
	static CAdvancedOptions*	s_singleton;

	HWND				m_parent;
	CConfig*			m_config;
	bool				m_isClient;
	CString				m_screenName;
	int					m_port;
	CString				m_interface;
};

#endif

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

#ifndef CGLOBALOPTIONS_H
#define CGLOBALOPTIONS_H

#include "CString.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

class CConfig;

//! Global options dialog for Microsoft Windows launcher
class CGlobalOptions {
public:
	CGlobalOptions(HWND parent, CConfig*);
	~CGlobalOptions();

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


	//@}

private:
	void				init(HWND hwnd);
	bool				save(HWND hwnd);

	int					getTime(HWND hwnd, HWND child, bool reportError);

	// message handling
	BOOL				doDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);

private:
	static CGlobalOptions*	s_singleton;

	HWND				m_parent;
	CConfig*			m_config;
	int					m_delayTime;
	int					m_twoTapTime;
	int					m_heartbeatTime;
};

#endif

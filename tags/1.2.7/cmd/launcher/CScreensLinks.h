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

#ifndef CSCREENSLINKS_H
#define CSCREENSLINKS_H

#include "CConfig.h"
#include "ProtocolTypes.h"
#include "CString.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

//! Screens and links dialog for Microsoft Windows launcher
class CScreensLinks {
public:
	CScreensLinks(HWND parent, CConfig*);
	~CScreensLinks();

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

	CString				getSelectedScreen(HWND hwnd) const;
	void				addScreen(HWND hwnd);
	void				editScreen(HWND hwnd);
	void				removeScreen(HWND hwnd);
	void				changeNeighbor(HWND hwnd,
							HWND combo, EDirection direction);

	void				updateScreens(HWND hwnd, const CString& name);
	void				updateScreensControls(HWND hwnd);
	void				updateLinksControls(HWND hwnd);
	void				updateLink(HWND hwnd,
							const CString& screen, EDirection direction);

	// message handling
	BOOL				doDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);

private:
	static CScreensLinks*	s_singleton;

	HWND				m_parent;
	CConfig*			m_mainConfig;
	CConfig				m_scratchConfig;
	CConfig*			m_config;
};

#endif

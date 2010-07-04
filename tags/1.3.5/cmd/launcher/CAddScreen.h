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

#ifndef CADDSCREEN_H
#define CADDSCREEN_H

#include "CString.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

class CConfig;

//! Add screen dialog for Microsoft Windows launcher
class CAddScreen {
public:
	CAddScreen(HWND parent, CConfig*, const CString& name);
	~CAddScreen();

	//! @name manipulators
	//@{

	//! Run dialog
	/*!
	Display and handle the dialog until closed by the user.  Return
	\c true if the user accepted the changes, false otherwise.
	*/
	bool				doModal();

	//@}
	//! @name accessors
	//@{

	CString				getName() const;

	//@}

private:
	typedef std::vector<CString> CStringList;

	void				getAliases(CStringList&) const;
	void				getOptions(CConfig::CScreenOptions&) const;

	static void			tokenize(CStringList& tokens, const CString& src);
	static bool			isNameInList(const CStringList& tokens,
							const CString& src);

	void				init(HWND hwnd);
	bool				save(HWND hwnd);

	// message handling
	BOOL				doDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);

private:
	static CAddScreen*	s_singleton;

	HWND				m_parent;
	CConfig*			m_config;
	CString				m_name;
};

#endif

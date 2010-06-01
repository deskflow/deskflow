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

#ifndef CMSWINDOWSCLIENTTASKBARRECEIVER_H
#define CMSWINDOWSCLIENTTASKBARRECEIVER_H

#define WIN32_LEAN_AND_MEAN

#include "CClientTaskBarReceiver.h"
#include <windows.h>

class CBufferedLogOutputter;

//! Implementation of CClientTaskBarReceiver for Microsoft Windows
class CMSWindowsClientTaskBarReceiver : public CClientTaskBarReceiver {
public:
	CMSWindowsClientTaskBarReceiver(HINSTANCE, const CBufferedLogOutputter*);
	virtual ~CMSWindowsClientTaskBarReceiver();

	// IArchTaskBarReceiver overrides
	virtual void		showStatus();
	virtual void		runMenu(int x, int y);
	virtual void		primaryAction();
	virtual const Icon	getIcon() const;
	void cleanup();

protected:
	void				copyLog() const;

	// CClientTaskBarReceiver overrides
	virtual void		onStatusChanged();

private:
	HICON				loadIcon(UINT);
	void				deleteIcon(HICON);
	void				createWindow();
	void				destroyWindow();

	BOOL				dlgProc(HWND hwnd,
							UINT msg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK
						staticDlgProc(HWND hwnd,
							UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE			m_appInstance;
	HWND				m_window;
	HMENU				m_menu;
	HICON				m_icon[kMaxState];
	const CBufferedLogOutputter*	m_logBuffer;
	static const UINT	s_stateToIconID[];
};

#endif

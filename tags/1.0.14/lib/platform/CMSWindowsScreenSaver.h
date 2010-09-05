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

#ifndef CMSWINDOWSSCREENSAVER_H
#define CMSWINDOWSSCREENSAVER_H

#include "IScreenSaver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CThread;

//! Microsoft windows screen saver implementation
class CMSWindowsScreenSaver : public IScreenSaver {
public:
	CMSWindowsScreenSaver();
	virtual ~CMSWindowsScreenSaver();

	//! @name manipulators
	//@{

	//! Check if screen saver started
	/*!
	Check if the screen saver really started.  Returns false if it
	hasn't, true otherwise.  When the screen saver stops, \c msg will
	be posted to the current thread's message queue with the given
	parameters.
	*/
	bool				checkStarted(UINT msg, WPARAM, LPARAM);

	//@}

	// IScreenSaver overrides
	virtual void		enable();
	virtual void		disable();
	virtual void		activate();
	virtual void		deactivate();
	virtual bool		isActive() const;

private:
	class CFindScreenSaverInfo {
	public:
		HDESK			m_desktop;
		HWND			m_window;
	};

	static BOOL CALLBACK	findScreenSaverFunc(HWND hwnd, LPARAM lParam);
	static BOOL CALLBACK	killScreenSaverFunc(HWND hwnd, LPARAM lParam);

	DWORD				findScreenSaver();
	void				watchDesktop();
	void				watchProcess(HANDLE process);
	void				unwatchProcess();
	void				watchDesktopThread(void*);
	void				watchProcessThread(void*);

private:
	bool				m_is95Family;
	bool				m_is95;
	bool				m_isNT;
	BOOL				m_wasEnabled;

	HANDLE				m_process;
	CThread*			m_watch;
	DWORD				m_threadID;
	UINT				m_msg;
	WPARAM				m_wParam;
	LPARAM				m_lParam;
};

#endif

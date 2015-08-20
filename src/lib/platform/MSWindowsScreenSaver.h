/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "synergy/IScreenSaver.h"
#include "base/String.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Thread;

//! Microsoft windows screen saver implementation
class MSWindowsScreenSaver : public IScreenSaver {
public:
	MSWindowsScreenSaver();
	virtual ~MSWindowsScreenSaver();

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
	class FindScreenSaverInfo {
	public:
		HDESK			m_desktop;
		HWND			m_window;
	};

	static BOOL CALLBACK	killScreenSaverFunc(HWND hwnd, LPARAM lParam);

	void				watchDesktop();
	void				watchProcess(HANDLE process);
	void				unwatchProcess();
	void				watchDesktopThread(void*);
	void				watchProcessThread(void*);

	void				setSecure(bool secure, bool saveSecureAsInt);
	bool				isSecure(bool* wasSecureAnInt) const;

private:
	BOOL				m_wasEnabled;
	bool				m_wasSecure;
	bool				m_wasSecureAnInt;

	HANDLE				m_process;
	Thread*			m_watch;
	DWORD				m_threadID;
	UINT				m_msg;
	WPARAM				m_wParam;
	LPARAM				m_lParam;

	// checkActive state.  true if the screen saver is being watched
	// for deactivation (and is therefore active).
	bool				m_active;
};

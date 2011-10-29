/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CARCHTASKBARWINDOWS_H
#define CARCHTASKBARWINDOWS_H

#define WIN32_LEAN_AND_MEAN

#include "IArchTaskBar.h"
#include "IArchMultithread.h"
#include "stdmap.h"
#include "stdvector.h"
#include <windows.h>

#define ARCH_TASKBAR CArchTaskBarWindows

//! Win32 implementation of IArchTaskBar
class CArchTaskBarWindows : public IArchTaskBar {
public:
	CArchTaskBarWindows(void*);
	virtual ~CArchTaskBarWindows();

	//! Add a dialog window 
	/*!
	Tell the task bar event loop about a dialog.  Win32 annoyingly
	requires messages destined for modeless dialog boxes to be
	dispatched differently than other messages.
	*/
	static void			addDialog(HWND);

	//! Remove a dialog window
	/*!
	Remove a dialog window added via \c addDialog().
	*/
	static void			removeDialog(HWND);

	// IArchTaskBar overrides
	virtual void		addReceiver(IArchTaskBarReceiver*);
	virtual void		removeReceiver(IArchTaskBarReceiver*);
	virtual void		updateReceiver(IArchTaskBarReceiver*);

private:
	class CReceiverInfo {
	public:
		UINT			m_id;
	};

	typedef std::map<IArchTaskBarReceiver*, CReceiverInfo> CReceiverToInfoMap;
	typedef std::map<UINT, CReceiverToInfoMap::iterator> CIDToReceiverMap;
	typedef std::vector<UINT> CIDStack;
	typedef std::map<HWND, bool> CDialogs;

	UINT				getNextID();
	void				recycleID(UINT);

	void				addIcon(UINT);
	void				removeIcon(UINT);
	void				updateIcon(UINT);
	void				addAllIcons();
	void				removeAllIcons();
	void				modifyIconNoLock(CReceiverToInfoMap::const_iterator,
							DWORD taskBarMessage);
	void				removeIconNoLock(UINT id);
	void				handleIconMessage(IArchTaskBarReceiver*, LPARAM);

	bool				processDialogs(MSG*);
	LRESULT				wndProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK
						staticWndProc(HWND, UINT, WPARAM, LPARAM);
	void				threadMainLoop();
	static void*		threadEntry(void*);

private:
	static CArchTaskBarWindows*	s_instance;
	static HINSTANCE	s_appInstance;

	// multithread data
	CArchMutex			m_mutex;
	CArchCond			m_condVar;
	bool				m_ready;
	int					m_result;
	CArchThread			m_thread;

	// child thread data
	HWND				m_hwnd;
	UINT				m_taskBarRestart;

	// shared data
	CReceiverToInfoMap	m_receivers;
	CIDToReceiverMap	m_idTable;
	CIDStack			m_oldIDs;
	UINT				m_nextID;

	// dialogs
	CDialogs			m_dialogs;
	CDialogs			m_addedDialogs;
};

#endif

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2003 Chris Schoeneman
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

#include "synergy/PortableTaskBarReceiver.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class BufferedLogOutputter;
class IEventQueue;

//! Implementation of PortableTaskBarReceiver for Microsoft Windows
class MSWindowsPortableTaskBarReceiver : public PortableTaskBarReceiver {
public:
	MSWindowsPortableTaskBarReceiver(HINSTANCE, const BufferedLogOutputter*, IEventQueue* events);
	virtual ~MSWindowsPortableTaskBarReceiver();

	// IArchTaskBarReceiver overrides
	virtual void		showStatus();
	virtual void		runMenu(int x, int y);
	virtual void		primaryAction();
	virtual const Icon	getIcon() const;
	void cleanup();

protected:
	void				copyLog() const;

	// PortableTaskBarReceiver overrides
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
	const BufferedLogOutputter*	m_logBuffer;
	IEventQueue*		m_events;

	static const UINT	s_stateToIconID[];
};

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#pragma once

#include "ILogOutputter.h"
#include "CArch.h"
#include <queue>
#include "IArchMultithread.h"

class CIpcServer;
class CEvent;
class CIpcClientProxy;

//! Write log to GUI over IPC
/*!
This outputter writes output to the GUI via IPC.
*/
class CIpcLogOutputter : public ILogOutputter {
public:
	CIpcLogOutputter(CIpcServer& ipcServer);
	virtual ~CIpcLogOutputter();

	// ILogOutputter overrides
	virtual void		open(const char* title);
	virtual void		close();
	virtual void		show(bool showIfEmpty);
	virtual bool		write(ELevel level, const char* message);

	//! Same as write, but allows message to sidestep anti-recursion mechanism.
	bool				write(ELevel level, const char* text, bool force);

	//! Notify that the buffer should be sent.
	void				notifyBuffer();

private:
	void				bufferThread(void*);
	CString				getChunk(size_t count);
	void				sendBuffer();
	void				appendBuffer(const CString& text);

private:
	typedef std::queue<CString> CBuffer;

	CIpcServer&			m_ipcServer;
	CBuffer				m_buffer;
	CArchMutex			m_bufferMutex;
	bool				m_sending;
	CThread*			m_bufferThread;
	bool				m_running;
	CArchCond			m_notifyCond;
	CArchMutex			m_notifyMutex;
	bool				m_bufferWaiting;
	IArchMultithread::ThreadID
						m_bufferThreadId;
};

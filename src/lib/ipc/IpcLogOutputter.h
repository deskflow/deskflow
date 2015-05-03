/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2012 Nick Bolton
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
 */

#pragma once

#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "base/ILogOutputter.h"

#include <queue>

class IpcServer;
class Event;
class IpcClientProxy;

//! Write log to GUI over IPC
/*!
This outputter writes output to the GUI via IPC.
*/
class IpcLogOutputter : public ILogOutputter {
public:
	IpcLogOutputter(IpcServer& ipcServer);
	virtual ~IpcLogOutputter();

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
	String				getChunk(size_t count);
	void				sendBuffer();
	void				appendBuffer(const String& text);

private:
	typedef std::queue<String> Buffer;

	IpcServer&			m_ipcServer;
	Buffer				m_buffer;
	ArchMutex			m_bufferMutex;
	bool				m_sending;
	Thread*			m_bufferThread;
	bool				m_running;
	ArchCond			m_notifyCond;
	ArchMutex			m_notifyMutex;
	bool				m_bufferWaiting;
	IArchMultithread::ThreadID
						m_bufferThreadId;
};

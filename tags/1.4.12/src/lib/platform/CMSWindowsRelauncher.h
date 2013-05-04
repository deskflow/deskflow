/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2009 Chris Schoeneman
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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <list>

class CThread;
class CIpcLogOutputter;
class CIpcServer;

class CMSWindowsRelauncher {
public:
	CMSWindowsRelauncher(
		bool autoDetectCommand,
		CIpcServer& ipcServer,
		CIpcLogOutputter& ipcLogOutputter);
	virtual ~CMSWindowsRelauncher();
	void startAsync();
	std::string command() const;
	void command(const std::string& command, bool elevate);
	void stop();

private:
	void mainLoop(void*);
	BOOL isProcessInSession(const char* name, DWORD sessionId, PHANDLE process);
	DWORD getSessionId();
	void outputLoop(void*);
	void shutdownProcess(HANDLE handle, DWORD pid, int timeout);
	void shutdownExistingProcesses();
	HANDLE				duplicateProcessToken(HANDLE process, LPSECURITY_ATTRIBUTES security);
	HANDLE				getSessionToken(DWORD sessionId, LPSECURITY_ATTRIBUTES security);
	HANDLE				getUserToken(DWORD sessionId, LPSECURITY_ATTRIBUTES security);

private:
	CThread* m_thread;
	bool m_autoDetectCommand;
	std::string m_command;
	bool m_running;
	bool m_commandChanged;
	HANDLE m_stdOutWrite;
	HANDLE m_stdOutRead;
	CThread* m_outputThread;
	CIpcServer&			m_ipcServer;
	CIpcLogOutputter&	m_ipcLogOutputter;
	bool				m_elevateProcess;
};

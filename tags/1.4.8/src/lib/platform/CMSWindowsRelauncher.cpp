/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2009 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CMSWindowsRelauncher.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CLog.h"
#include "CArch.h"
#include "Version.h"
#include "CArchDaemonWindows.h"
#include "XArchWindows.h"
#include "CApp.h"
#include "CArgsBase.h"

#include <Tlhelp32.h>
#include <UserEnv.h>
#include <sstream>

enum {
	kOutputBufferSize = 4096
};

typedef VOID (WINAPI *SendSas)(BOOL asUser);

CMSWindowsRelauncher::CMSWindowsRelauncher(bool autoDetectCommand) :
	m_thread(NULL),
	m_autoDetectCommand(autoDetectCommand),
	m_running(true),
	m_commandChanged(false),
	m_stdOutWrite(NULL),
	m_stdOutRead(NULL)
{
}

CMSWindowsRelauncher::~CMSWindowsRelauncher()
{
}

void 
CMSWindowsRelauncher::startAsync()
{
	m_thread = new CThread(new TMethodJob<CMSWindowsRelauncher>(
		this, &CMSWindowsRelauncher::mainLoop, nullptr));

	m_outputThread = new CThread(new TMethodJob<CMSWindowsRelauncher>(
		this, &CMSWindowsRelauncher::outputLoop, nullptr));
}

void
CMSWindowsRelauncher::stop()
{
	m_running = false;
	m_thread->wait(5);
}

// this still gets the physical session (the one the keyboard and 
// mouse is connected to), sometimes this returns -1 but not sure why
DWORD 
CMSWindowsRelauncher::getSessionId()
{
	return WTSGetActiveConsoleSessionId();
}

BOOL
CMSWindowsRelauncher::winlogonInSession(DWORD sessionId, PHANDLE process)
{
	// first we need to take a snapshot of the running processes
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		LOG((CLOG_ERR "could not get process snapshot (error: %i)", 
			GetLastError()));
		return 0;
	}

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	// get the first process, and if we can't do that then it's 
	// unlikely we can go any further
	BOOL gotEntry = Process32First(snapshot, &entry);
	if (!gotEntry) {
		LOG((CLOG_ERR "could not get first process entry (error: %i)", 
			GetLastError()));
		return 0;
	}

	// used to record process names for debug info
	std::list<std::string> nameList;

	// now just iterate until we can find winlogon.exe pid
	DWORD pid = 0;
	while(gotEntry) {

		// make sure we're not checking the system process
		if (entry.th32ProcessID != 0) {

			DWORD processSessionId;
			BOOL pidToSidRet = ProcessIdToSessionId(
				entry.th32ProcessID, &processSessionId);

			if (!pidToSidRet) {
				LOG((CLOG_ERR "could not get session id for process id %i (error: %i)",
					entry.th32ProcessID, GetLastError()));
				return 0;
			}

			// only pay attention to processes in the active session
			if (processSessionId == sessionId) {

				// store the names so we can record them for debug
				nameList.push_back(entry.szExeFile);

				if (_stricmp(entry.szExeFile, "winlogon.exe") == 0) {
					pid = entry.th32ProcessID;
					break;
				}
			}
		}

		// now move on to the next entry (if we're not at the end)
		gotEntry = Process32Next(snapshot, &entry);
		if (!gotEntry) {

			DWORD err = GetLastError();
			if (err != ERROR_NO_MORE_FILES) {

				// only worry about error if it's not the end of the snapshot
				LOG((CLOG_ERR "could not get subsiquent process entry (error: %i)", 
					GetLastError()));
				return 0;
			}
		}
	}

	std::string nameListJoin;
	for(std::list<std::string>::iterator it = nameList.begin();
		it != nameList.end(); it++) {
			nameListJoin.append(*it);
			nameListJoin.append(", ");
	}

	LOG((CLOG_DEBUG "checked processes while looking for winlogon.exe: %s",
		nameListJoin.c_str()));

	CloseHandle(snapshot);

	if (pid) {
		// now get the process so we can get the process, with which
		// we'll use to get the process token.
		*process = OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
		return true;
	}
	else {
		LOG((CLOG_DEBUG "could not find winlogon.exe in session %i", sessionId));
		return false;
	}
}

// gets the current user (so we can launch under their session)
HANDLE 
CMSWindowsRelauncher::getCurrentUserToken(DWORD sessionId, LPSECURITY_ATTRIBUTES security)
{
	HANDLE currentToken;
	HANDLE winlogonProcess;

	if (winlogonInSession(sessionId, &winlogonProcess)) {

		LOG((CLOG_DEBUG "session %i has winlogon.exe", sessionId));

		// get the token, so we can re-launch with this token
		// -- do we really need all these access bits?
		BOOL tokenRet = OpenProcessToken(
			winlogonProcess,
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | 
			TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | 
			TOKEN_ADJUST_SESSIONID | TOKEN_READ | TOKEN_WRITE,
			&currentToken);
	}
	else {

		LOG((CLOG_ERR "session %i does not have winlogon.exe "
			"which is needed for re-launch", sessionId));
		return 0;
	}

	HANDLE primaryToken;
	BOOL duplicateRet = DuplicateTokenEx(
		currentToken, MAXIMUM_ALLOWED, security,
		SecurityImpersonation, TokenPrimary, &primaryToken);

	if (!duplicateRet) {
		LOG((CLOG_ERR "could not duplicate token %i (error: %i)",
			currentToken, GetLastError()));
		return 0;
	}

	return primaryToken;
}

void
CMSWindowsRelauncher::mainLoop(void*)
{
	SendSas sendSasFunc = NULL;
	HINSTANCE sasLib = LoadLibrary("sas.dll");
	if (sasLib) {
		LOG((CLOG_DEBUG "found sas.dll"));
		sendSasFunc = (SendSas)GetProcAddress(sasLib, "SendSAS");
	}

	DWORD sessionId = -1;
	bool launched = false;

	SECURITY_ATTRIBUTES saAttr; 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	if (!CreatePipe(&m_stdOutRead, &m_stdOutWrite, &saAttr, 0)) {
		throw XArch(new XArchEvalWindows());
	}

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	while (m_running) {

		HANDLE sendSasEvent = 0;
		if (sasLib && sendSasFunc) {
			// can't we just create one event? seems weird creating a new
			// event every second...
			sendSasEvent = CreateEvent(NULL, FALSE, FALSE, "Global\\SendSAS");
		}

		DWORD newSessionId = getSessionId();

		// only enter here when id changes, and the session isn't -1, which
		// may mean that there is no active session.
		if (((newSessionId != sessionId) && (newSessionId != -1)) || m_commandChanged) {
			
			m_commandChanged = false;

			if (launched) {
				LOG((CLOG_DEBUG "closing existing process to make way for new one"));
				shutdownProcess(pi, 10);
				launched = false;
			}

			// ok, this is now the active session (forget the old one if any)
			sessionId = newSessionId;

			SECURITY_ATTRIBUTES sa;
			ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));

			// get the token for the user in active session, which is the
			// one receiving input from mouse and keyboard.
			HANDLE userToken = getCurrentUserToken(sessionId, &sa);

			if (userToken != 0) {
				LOG((CLOG_DEBUG "got user token to launch new process"));

				std::string cmd = command();
				if (cmd == "") {
					LOG((CLOG_WARN "nothing to launch, no command specified."));
					continue;
				}

				// in case reusing process info struct
				ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

				STARTUPINFO si;
				ZeroMemory(&si, sizeof(STARTUPINFO));
				si.cb = sizeof(STARTUPINFO);
				si.lpDesktop = "winsta0\\default";
				si.hStdError = m_stdOutWrite;
				si.hStdOutput = m_stdOutWrite;
				si.dwFlags |= STARTF_USESTDHANDLES;

				LPVOID environment;
				BOOL blockRet = CreateEnvironmentBlock(&environment, userToken, FALSE);
				if (!blockRet) {
					LOG((CLOG_ERR "could not create environment block (error: %i)", 
						GetLastError()));
					continue;
				}
				else {

					DWORD creationFlags = 
						NORMAL_PRIORITY_CLASS |
						CREATE_NO_WINDOW |
						CREATE_UNICODE_ENVIRONMENT;

					// re-launch in current active user session
					BOOL createRet = CreateProcessAsUser(
						userToken, NULL, LPSTR(cmd.c_str()),
						&sa, NULL, TRUE, creationFlags,
						environment, NULL, &si, &pi);

					DestroyEnvironmentBlock(environment);
					CloseHandle(userToken);

					if (!createRet) {
						LOG((CLOG_ERR "could not launch (error: %i)", GetLastError()));
						continue;
					}
					else {
						LOG((CLOG_DEBUG "launched in session %i (cmd: %s)", 
							sessionId, cmd.c_str()));
						launched = true;
					}
				}
			}
		}

		if (sendSasEvent) {
			// use SendSAS event to wait for next session.
			if (WaitForSingleObject(sendSasEvent, 1000) == WAIT_OBJECT_0 && sendSasFunc) {
				LOG((CLOG_DEBUG "calling SendSAS"));
				sendSasFunc(FALSE);
			}
			CloseHandle(sendSasEvent);
		}
		else {
			// check for session change every second.
			ARCH->sleep(1);
		}
	}

	if (launched) {
		LOG((CLOG_DEBUG "terminated running process on exit"));
		shutdownProcess(pi, 10);
	}
}

void
CMSWindowsRelauncher::command(const std::string& command)
{
	LOG((CLOG_INFO "service command updated"));
	LOG((CLOG_DEBUG "new command: %s", command.c_str()));
	m_command = command;
	m_commandChanged = true;
}

std::string
CMSWindowsRelauncher::command() const
{
	if (!m_autoDetectCommand) {
		return m_command;
	}

	// seems like a fairly convoluted way to get the process name
	const char* launchName = CApp::instance().argsBase().m_pname;
	std::string args = ARCH->commandLine();

	// build up a full command line
	std::stringstream cmdTemp;
	cmdTemp << launchName << /*" --debug-data session-" << sessionId <<*/ args;

	std::string cmd = cmdTemp.str();

	size_t i;
	std::string find = "--relaunch";
	while((i = cmd.find(find)) != std::string::npos) {
		cmd.replace(i, find.length(), "");
	}

	return cmd;
}

void
CMSWindowsRelauncher::outputLoop(void*)
{
	CHAR buffer[kOutputBufferSize];

	while (true) {
		
		DWORD bytesRead;
		BOOL success = ReadFile(m_stdOutRead, buffer, kOutputBufferSize, &bytesRead, NULL);

		// assume the process has gone away? slow down
		// the reads until another one turns up.
		if (!success || bytesRead == 0) {
			ARCH->sleep(1);
		}
		else {
			// send process output over IPC to GUI.
			buffer[bytesRead] = '\0';
			ARCH->ipcLog().writeLog(kINFO, buffer);
		}
			
	}
}

void
CMSWindowsRelauncher::shutdownProcess(const PROCESS_INFORMATION& pi, int timeout)
{
	DWORD exitCode;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	if (exitCode != STILL_ACTIVE)
		return;

	sendIpcMessage(kIpcShutdown, "");

	// wait for process to exit gracefully.
	double start = ARCH->time();
	while (true)
	{
		GetExitCodeProcess(pi.hProcess, &exitCode);
		if (exitCode != STILL_ACTIVE) {
			LOG((CLOG_INFO "process %d was shutdown successfully", pi.dwProcessId));
			break;
		}
		else {
			
			if ((ARCH->time() - start) > timeout) {
				// if timeout reached, kill forcefully.
				// calling TerminateProcess on synergy is very bad!
				// it causes the hook DLL to stay loaded in some apps,
				// making it impossible to start synergy again.
				LOG((CLOG_WARN "shutdown timed out after %d secs, forcefully terminating", pi.dwProcessId));
				TerminateProcess(pi.hProcess, kExitSuccess);
				break;
			}

			ARCH->sleep(1);
		}
	}
}

// TODO: put this in an IPC client class.
void
CMSWindowsRelauncher::sendIpcMessage(int type, const char* data)
{
	char message[1024];
	message[0] = type;
	char* messagePtr = message;
	messagePtr++;
	strcpy(messagePtr, data);

	HANDLE pipe = CreateFile(
		_T("\\\\.\\pipe\\SynergyNode"),
		GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (pipe == INVALID_HANDLE_VALUE)
	{
		LOG((CLOG_ERR "could not connect to node, error: %d", GetLastError()));
		return;
	}

	DWORD dwMode = PIPE_READMODE_MESSAGE;
	BOOL stateSuccess = SetNamedPipeHandleState(pipe, &dwMode, NULL, NULL);

	if (!stateSuccess)
	{
		LOG((CLOG_ERR "could not set node pipe state, error: %d", GetLastError()));
		return;
	}

	DWORD written;
	BOOL writeSuccess = WriteFile(
		pipe, message, (DWORD)strlen(message), &written, NULL);

	if (!writeSuccess)
	{
		LOG((CLOG_ERR "could not write to node pipe, error: %d", GetLastError()));
		return;
	}

	CloseHandle(pipe);
}

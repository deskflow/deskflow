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
#include "CIpcLogOutputter.h"
#include "CIpcServer.h"
#include "CIpcMessage.h"
#include "Ipc.h"

#include <Tlhelp32.h>
#include <UserEnv.h>
#include <sstream>
#include <Wtsapi32.h>
#include <Shellapi.h>

enum {
	kOutputBufferSize = 4096
};

typedef VOID (WINAPI *SendSas)(BOOL asUser);

CMSWindowsRelauncher::CMSWindowsRelauncher(
	bool autoDetectCommand,
	CIpcServer& ipcServer,
	CIpcLogOutputter& ipcLogOutputter) :
	m_thread(NULL),
	m_autoDetectCommand(autoDetectCommand),
	m_running(true),
	m_commandChanged(false),
	m_stdOutWrite(NULL),
	m_stdOutRead(NULL),
	m_ipcServer(ipcServer),
	m_ipcLogOutputter(ipcLogOutputter),
	m_elevateProcess(false)
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
	delete m_thread;

	m_outputThread->wait(5);
	delete m_outputThread;
}

// this still gets the physical session (the one the keyboard and 
// mouse is connected to), sometimes this returns -1 but not sure why
DWORD 
CMSWindowsRelauncher::getSessionId()
{
	return WTSGetActiveConsoleSessionId();
}

BOOL
CMSWindowsRelauncher::isProcessInSession(const char* name, DWORD sessionId, PHANDLE process = NULL)
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

				if (_stricmp(entry.szExeFile, name) == 0) {
					pid = entry.th32ProcessID;
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

	LOG((CLOG_DEBUG "processes in session %d: %s",
		sessionId, nameListJoin.c_str()));

	CloseHandle(snapshot);

	if (pid) {
		if (process != NULL) {
			// now get the process so we can get the process, with which
			// we'll use to get the process token.
			LOG((CLOG_DEBUG "found %s in session %i", name, sessionId));
			*process = OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
		}
		return true;
	}
	else {
		return false;
	}
}

HANDLE
CMSWindowsRelauncher::duplicateProcessToken(HANDLE process, LPSECURITY_ATTRIBUTES security)
{
	HANDLE sourceToken;

	BOOL tokenRet = OpenProcessToken(
		process,
		TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS,
		&sourceToken);

	if (!tokenRet) {
		LOG((CLOG_ERR "could not open token, process handle: %d (error: %i)", process, GetLastError()));
		return NULL;
	}
	
	LOG((CLOG_DEBUG "got token %i, duplicating", sourceToken));

	HANDLE newToken;
	BOOL duplicateRet = DuplicateTokenEx(
		sourceToken, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, security,
		SecurityImpersonation, TokenPrimary, &newToken);

	if (!duplicateRet) {
		LOG((CLOG_ERR "could not duplicate token %i (error: %i)",
			sourceToken, GetLastError()));
		return NULL;
	}
	
	LOG((CLOG_DEBUG "duplicated, new token: %i", newToken));
	return newToken;
}

HANDLE 
CMSWindowsRelauncher::getUserToken(DWORD sessionId, LPSECURITY_ATTRIBUTES security)
{
	// always elevate if we are at the vista/7 login screen. we could also 
	// elevate for the uac dialog (consent.exe) but this would be pointless,
	// since synergy would re-launch as non-elevated after the desk switch,
	// and so would be unusable with the new elevated process taking focus.
	if (m_elevateProcess || isProcessInSession("logonui.exe", sessionId)) {
		
		LOG((CLOG_DEBUG "getting elevated token, %s",
			(m_elevateProcess ? "elevation required" : "at login screen")));

		HANDLE process;
		if (isProcessInSession("winlogon.exe", sessionId, &process)) {
			return duplicateProcessToken(process, security);
		}
		else {
			LOG((CLOG_ERR "could not find winlogon in session %i", sessionId));
			return NULL;
		}
	}
	else {
		LOG((CLOG_DEBUG "getting non-elevated token"));
		return getSessionToken(sessionId, security);
	}
}

HANDLE 
CMSWindowsRelauncher::getSessionToken(DWORD sessionId, LPSECURITY_ATTRIBUTES security)
{
	HANDLE sourceToken;
	if (!WTSQueryUserToken(sessionId, &sourceToken)) {
		LOG((CLOG_ERR "could not get token from session %d (error: %i)", sessionId, GetLastError()));
		return 0;
	}
	
	HANDLE newToken;
	if (!DuplicateTokenEx(
		sourceToken, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, security,
		SecurityImpersonation, TokenPrimary, &newToken)) {

		LOG((CLOG_ERR "could not duplicate token (error: %i)", GetLastError()));
		return 0;
	}
	
	LOG((CLOG_DEBUG "duplicated, new token: %i", newToken));
	return newToken;
}

void
CMSWindowsRelauncher::mainLoop(void*)
{
	shutdownExistingProcesses();

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

	int failures = 0;

	while (m_running) {

		HANDLE sendSasEvent = 0;
		if (sasLib && sendSasFunc) {
			// can't we just create one event? seems weird creating a new
			// event every second...
			sendSasEvent = CreateEvent(NULL, FALSE, FALSE, "Global\\SendSAS");
		}

		DWORD newSessionId = getSessionId();

		bool running = false;
		if (launched) {

			DWORD exitCode;
			GetExitCodeProcess(pi.hProcess, &exitCode);
			running = (exitCode == STILL_ACTIVE);

			if (!running) {
				failures++;
				LOG((CLOG_INFO "detected application not running, pid=%d, failures=%d", pi.dwProcessId, failures));
				
				// increasing backoff period, maximum of 10 seconds.
				int timeout = (failures * 2) < 10 ? (failures * 2) : 10;
				LOG((CLOG_DEBUG "waiting, backoff period is %d seconds", timeout));
				ARCH->sleep(timeout);
				
				// double check, in case process started after we waited.
				GetExitCodeProcess(pi.hProcess, &exitCode);
				running = (exitCode == STILL_ACTIVE);
			}
			else {
				// reset failures when running.
				failures = 0;
			}
		}

		// only enter here when id changes, and the session isn't -1, which
		// may mean that there is no active session.
		bool sessionChanged = ((newSessionId != sessionId) && (newSessionId != -1));

		// relaunch if it was running but has stopped unexpectedly.
		bool stoppedRunning = (launched && !running);

		if (stoppedRunning || sessionChanged || m_commandChanged) {
			
			m_commandChanged = false;

			if (launched) {
				LOG((CLOG_DEBUG "closing existing process to make way for new one"));
				shutdownProcess(pi.hProcess, pi.dwProcessId, 20);
				launched = false;
			}

			// ok, this is now the active session (forget the old one if any)
			sessionId = newSessionId;

			SECURITY_ATTRIBUTES sa;
			ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));

			HANDLE userToken = getUserToken(sessionId, &sa);
			if (userToken == NULL) {
				// HACK: trigger retry mechanism.
				launched = true;
				continue;
			}

			std::string cmd = command();
			if (cmd == "") {
				// this appears on first launch when the user hasn't configured
				// anything yet, so don't show it as a warning, only show it as
				// debug to devs to let them know why nothing happened.
				LOG((CLOG_DEBUG "nothing to launch, no command specified."));
				continue;
			}

			// in case reusing process info struct
			ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

			STARTUPINFO si;
			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			si.lpDesktop = "winsta0\\Default"; // TODO: maybe this should be \winlogon if we have logonui.exe?
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

			DWORD creationFlags = 
				NORMAL_PRIORITY_CLASS |
				CREATE_NO_WINDOW |
				CREATE_UNICODE_ENVIRONMENT;

			// re-launch in current active user session
			LOG((CLOG_INFO "starting new process"));
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
		shutdownProcess(pi.hProcess, pi.dwProcessId, 20);
	}
	
	LOG((CLOG_DEBUG "relauncher main thread finished"));
}

void
CMSWindowsRelauncher::command(const std::string& command, bool elevate)
{
	LOG((CLOG_INFO "service command updated"));
	m_command = command;
	m_elevateProcess = elevate;
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
	cmdTemp << launchName << args;

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
	// +1 char for \0
	CHAR buffer[kOutputBufferSize + 1];

	while (m_running) {
		
		DWORD bytesRead;
		BOOL success = ReadFile(m_stdOutRead, buffer, kOutputBufferSize, &bytesRead, NULL);

		// assume the process has gone away? slow down
		// the reads until another one turns up.
		if (!success || bytesRead == 0) {
			ARCH->sleep(1);
		}
		else {
			buffer[bytesRead] = '\0';

			// send process output over IPC to GUI, and force it to be sent
			// which bypasses the ipc logging anti-recursion mechanism.
			m_ipcLogOutputter.write(kINFO, buffer, true);
		}
			
	}
}

void
CMSWindowsRelauncher::shutdownProcess(HANDLE handle, DWORD pid, int timeout)
{
	DWORD exitCode;
	GetExitCodeProcess(handle, &exitCode);
	if (exitCode != STILL_ACTIVE)
		return;

	CIpcShutdownMessage shutdown;
	m_ipcServer.send(shutdown, kIpcClientNode);

	// wait for process to exit gracefully.
	double start = ARCH->time();
	while (true)
	{
		GetExitCodeProcess(handle, &exitCode);
		if (exitCode != STILL_ACTIVE) {
			// yay, we got a graceful shutdown. there should be no hook in use errors!
			LOG((CLOG_INFO "process %d was shutdown gracefully", pid));
			break;
		}
		else {
			
			double elapsed = (ARCH->time() - start);
			if (elapsed > timeout) {
				// if timeout reached, kill forcefully.
				// calling TerminateProcess on synergy is very bad!
				// it causes the hook DLL to stay loaded in some apps,
				// making it impossible to start synergy again.
				LOG((CLOG_WARN "shutdown timed out after %d secs, forcefully terminating", (int)elapsed));
				TerminateProcess(handle, kExitSuccess);
				break;
			}

			ARCH->sleep(1);
		}
	}
}

void
CMSWindowsRelauncher::shutdownExistingProcesses()
{
	// first we need to take a snapshot of the running processes
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		LOG((CLOG_ERR "could not get process snapshot (error: %i)", 
			GetLastError()));
		return;
	}

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	// get the first process, and if we can't do that then it's 
	// unlikely we can go any further
	BOOL gotEntry = Process32First(snapshot, &entry);
	if (!gotEntry) {
		LOG((CLOG_ERR "could not get first process entry (error: %i)", 
			GetLastError()));
		return;
	}

	// now just iterate until we can find winlogon.exe pid
	DWORD pid = 0;
	while(gotEntry) {

		// make sure we're not checking the system process
		if (entry.th32ProcessID != 0) {

			if (_stricmp(entry.szExeFile, "synergyc.exe") == 0 ||
				_stricmp(entry.szExeFile, "synergys.exe") == 0) {
				
				HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				shutdownProcess(handle, entry.th32ProcessID, 10);
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
				return;
			}
		}
	}

	CloseHandle(snapshot);
}

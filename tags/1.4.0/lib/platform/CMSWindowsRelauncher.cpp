#include "CMSWindowsRelauncher.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CLog.h"
#include "CArch.h"
#include "Version.h"
#include "CArchDaemonWindows.h"

#include <Tlhelp32.h>
#include <UserEnv.h>
#include <sstream>

CMSWindowsRelauncher::CMSWindowsRelauncher()
{
	
}

CMSWindowsRelauncher::~CMSWindowsRelauncher()
{
	delete m_thread;
}

void 
CMSWindowsRelauncher::startAsync()
{
	m_thread = new CThread(new TMethodJob<CMSWindowsRelauncher>(
		this, &CMSWindowsRelauncher::startThread, nullptr));
}

void 
CMSWindowsRelauncher::startThread(void*)
{
	LOG((CLOG_DEBUG "starting relaunch loop"));
	int ret = relaunchLoop();

	// HACK: this actually throws an exception to exit with 0 (nasty)
	ARCH->util().app().m_bye(ret);
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

int
CMSWindowsRelauncher::relaunchLoop()
{
	// start with invalid id (gets re-assigned on first loop)
	DWORD sessionId = -1;

	// keep here so we can check if proc running -- huh?
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	int returnCode = kExitSuccess;
	bool launched = false;

	// TODO: fix this hack BEFORE release; we need to exit gracefully instead 
	// of being force killed!
	bool loopRunning = true;
	while (loopRunning) {

		DWORD newSessionId = getSessionId();

		// only enter here when id changes, and the session isn't -1, which
		// may mean that there is no active session.
		if ((newSessionId != sessionId) && (newSessionId != -1)) {

			// HACK: doesn't close process in a nice way
			// TODO: use CloseMainWindow instead
			if (launched) {
				TerminateProcess(pi.hProcess, kExitSuccess);
				LOG((CLOG_DEBUG "terminated existing process to make way for new one"));
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

				std::string cmd = getCommand();

				// in case reusing process info struct
				ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

				STARTUPINFO si;
				ZeroMemory(&si, sizeof(STARTUPINFO));
				si.cb = sizeof(STARTUPINFO);
				si.lpDesktop = "winsta0\\default";

				LPVOID environment;
				BOOL blockRet = CreateEnvironmentBlock(&environment, userToken, FALSE);
				if (!blockRet) {
					LOG((CLOG_ERR "could not create environment block (error: %i)", 
						GetLastError()));

					returnCode = kExitFailed;
					loopRunning = false; // stop loop
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
						returnCode = kExitFailed;
						loopRunning = false;
					}
					else {
						LOG((CLOG_DEBUG "launched in session %i (cmd: %s)", 
							sessionId, cmd.c_str()));
						launched = true;
					}
				}
			}
		}

		// check for session change every second
		ARCH->sleep(1);
	}

	if (launched) {
		// HACK: kill just in case process it has survived somehow
		TerminateProcess(pi.hProcess, kExitSuccess);
	}

	return kExitSuccess;
}

std::string CMSWindowsRelauncher::getCommand()
{
	// seems like a fairly convoluted way to get the process name
	const char* launchName = ARCH->util().app().argsBase().m_pname;
	std::string args = ((CArchDaemonWindows&)ARCH->daemon()).commandLine();

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

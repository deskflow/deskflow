#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

class CThread;

class CMSWindowsRelauncher {
public:
	CMSWindowsRelauncher();
	virtual ~CMSWindowsRelauncher();
	void startAsync();
	CThread* m_thread;
	void startThread(void*);
	BOOL winlogonInSession(DWORD sessionId, PHANDLE process);
	DWORD getSessionId();
	HANDLE getCurrentUserToken(DWORD sessionId, LPSECURITY_ATTRIBUTES security);
	int relaunchLoop();
	std::string getCommand();
};

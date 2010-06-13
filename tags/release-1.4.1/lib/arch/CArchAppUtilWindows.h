/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include "CArchAppUtil.h"
#include "CString.h"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#define ARCH_APPUTIL CArchAppUtilWindows

enum AppExitMode {
	kExitModeNormal,
	kExitModeDaemon
};

class CArchAppUtilWindows : public CArchAppUtil {
public:
	CArchAppUtilWindows();
	virtual ~CArchAppUtilWindows();

	// Gets the arguments to be used with a service.
	CString getServiceArgs() const;

	// Install application as Windows service.
	void installService();

	// Uninstall a Windows service with matching daemon name.
	void uninstallService();

	// Start a Windows service with matching daemon name.
	void startService();

	// Stop a Windows service with matching daemon name.
	void stopService();

	// Will install, uninstall, start, or stop the service depending on arg.
	void handleServiceArg(const char* serviceAction);

	bool parseArg(const int& argc, const char* const* argv, int& i);

	int daemonNTStartup(int, char**);
	
	int daemonNTMainLoop(int argc, const char** argv);

	void debugServiceWait();

	int run(int argc, char** argv);

	void exitApp(int code);

	void beforeAppExit();

	static CArchAppUtilWindows& instance();

	void startNode();

private:
	AppExitMode m_exitMode;
	static BOOL WINAPI consoleHandler(DWORD CEvent);
};

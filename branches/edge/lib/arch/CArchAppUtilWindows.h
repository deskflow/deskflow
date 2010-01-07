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

#include "Windows.h"

#define ARCH_APPUTIL CArchAppUtilWindows

class CArchAppUtilWindows : public CArchAppUtil {
public:
	CArchAppUtilWindows();
	virtual ~CArchAppUtilWindows();

	// Instance of MFC Windows application.
	static HINSTANCE s_instanceWin32;

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
	
	void adoptApp(CApp* app);

	int daemonNTStartup(int, char**);
	
	int daemonNTMainLoop(int argc, const char** argv);

	int run(int argc, char** argv, CreateTaskBarReceiverFunc createTaskBarReceiver);

	static void byeThrow(int x);

	static CArchAppUtilWindows& instance() { return (CArchAppUtilWindows&)*s_instance; }
};

// TODO: move to class
void exitPause(int code);

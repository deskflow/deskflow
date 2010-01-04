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

#include "CApp.h"

#include <Windows.h>

class CMSWindowsApp : public CAppBridge {
public:
	CMSWindowsApp();
	virtual ~CMSWindowsApp();

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

	// Instance of MFC Windows application.
	HINSTANCE m_instance;

protected:
	virtual void adoptParent(CApp* parent);
};

// TODO: move to class
void 
exitPause(int code);

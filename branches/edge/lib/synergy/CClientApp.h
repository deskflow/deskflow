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
#include "CString.h"
#include "CNetworkAddress.h"

class CClientApp : public CApp {
public:
	class CArgs : public CApp::CArgsBase {
	public:
		CArgs();
		~CArgs();

	public:
		int m_yscroll;
		CNetworkAddress* m_serverAddress;
	};

	CClientApp();
	virtual ~CClientApp();

	// Parse client specific command line arguments.
	void parseArgs(int argc, const char* const* argv);

	// Prints help specific to client.
	void help();

	// Returns arguments that are common and for client.
	CArgs& args() const { return (CArgs&)argsBase(); }

	// TODO: implement these for client app
	void loadConfig() { /* config not support for client */ }
	bool loadConfig(const CString& pathname) { return false; /* config not support for client */ }
	int mainLoop() { return 0; }
	int foregroundStartup(int argc, char** argv) { return 0; }
	int daemonMainLoop(int, const char**) { return 0; }
	int run(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup, CreateTaskBarReceiverFunc createTaskBarReceiver) { return 0; }

private:
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);
};

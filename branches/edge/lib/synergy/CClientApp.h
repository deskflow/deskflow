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

	// Static instance for backwards compat.
	static CClientApp* s_instance;

private:
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);
};

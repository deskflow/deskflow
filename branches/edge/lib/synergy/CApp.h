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

#include "common.h"
#include "CString.h"

class CApp {
public:
	class CArgsBase {
	public:
		CArgsBase();
		virtual ~CArgsBase();
		bool m_daemon;
		bool m_backend;
		bool m_restartable;
		bool m_noHooks;
		const char* m_pname;
		const char* m_logFilter;
		const char*	m_logFile;
		const char*	m_display;
		CString m_name;
	};

	CApp(CArgsBase* args);
	virtual ~CApp();

	// Returns args that are common between server and client.
	CArgsBase& argsBase() const { return *m_args; }

	// Prints the current compiled version.
	virtual void version();

	// Prints help specific to client or server.
	virtual void help() = 0;

	// Parse command line arguments.
	virtual void parseArgs(int argc, const char* const* argv) = 0;

	// Name of the daemon (used for Unix and Windows).
	CString m_daemonName;

	// A description of the daemon (used only on Windows).
	CString m_daemonInfo;

	// Function pointer for function to exit immediately.
	// TODO: this is old C code - use inheritance to normalize
	void (*m_bye)(int);

	// Returns true if argv[argi] is equal to name1 or name2.
	bool isArg(int argi, int argc, const char* const* argv,
		const char* name1, const char* name2,
		int minRequiredParameters = 0);

protected:
	virtual void parseArgs(int argc, const char* const* argv, int &i);
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);

private:
	CArgsBase* m_args;
};

#define BYE "\nTry `%s --help' for more information."

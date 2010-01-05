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

class CAppUtil;

class CApp {
public:
	class CArgsBase {
	public:
		CArgsBase();
		virtual ~CArgsBase();
		bool m_daemon;
		bool m_backend;
		bool m_restartable;
		const char* m_pname;
		const char* m_logFilter;
		const char*	m_logFile;
		const char*	m_display;
		CString m_name;
	};

	CApp(CArgsBase* args, CAppUtil* bridge);
	virtual ~CApp();

	// Returns args that are common between server and client.
	CArgsBase& argsBase() const { return *m_args; }

	// Returns a platform specific utility (base type).
	CAppUtil& utilBase() const { return *m_util; }

	// Prints the current compiled version.
	virtual void version();

	// Prints help specific to client or server.
	virtual void help() = 0;

	// Parse command line arguments.
	virtual void parse(int argc, const char* const* argv) = 0;

	// Name of the daemon (used for Unix and Windows).
	CString m_daemonName;

	// A description of the daemon (used only on Windows).
	CString m_daemonInfo;

	// Function pointer for function to exit immediately.
	// TODO: this is old C code - use inheritance to normalize
	void (*m_bye)(int);

protected:
	virtual void parse(int argc, const char* const* argv, int &i);
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);
	bool isArg(int argi, int argc, const char* const* argv,
		const char* name1, const char* name2,
		int minRequiredParameters = 0);

private:
	CArgsBase* m_args;
	CAppUtil* m_util;
};

#define BYE "\nTry `%s --help' for more information."

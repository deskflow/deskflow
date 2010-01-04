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
		CArgsBase() : m_pname(NULL) { }
		virtual ~CArgsBase() { }
		const char* m_pname;
	};

	CApp(CArgsBase* args, CAppUtil* bridge);
	virtual ~CApp();

	CArgsBase& argsBase() const { return *m_args; }
	CAppUtil& utilBase() const { return *m_util; }

	CString m_daemonName;
	CString m_daemonInfo;
	void (*m_bye)(int);

protected:
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);
	bool isArg(int argi, int argc, const char* const* argv,
		const char* name1, const char* name2,
		int minRequiredParameters = 0);

private:
	CArgsBase* m_args;
	CAppUtil* m_util;
};

#define BYE "\nTry `%s --help' for more information."

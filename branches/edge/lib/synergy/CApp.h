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

class CApp;

// The CAppBridge is so we can avoid using diamond inheritance; instead of
// both client and server platform implementations inheriting their platform
// app type and CServer/CClient, we simply use a reference (so we don't need
// to worry about virtual inheritance pitfalls).
// NOTE: we may be over-normalizing here; if so, we should consider just 
// passing variables to "platform app" classes instead.
class CAppBridge {
	friend class CApp;
public:
	CAppBridge() : m_parent(nullptr) { }
	virtual ~CAppBridge() { }
protected:
	CApp& parent() const { assert(m_parent != nullptr); return *m_parent; }
	virtual void adoptParent(CApp* parent) { m_parent = parent; }
private:
	CApp* m_parent;
};

class CApp {
public:
	class CArgsBase {
	public:
		CArgsBase() : m_pname(NULL) { }
		virtual ~CArgsBase() { }
		const char* m_pname;
	};

	CApp(CArgsBase* args, CAppBridge* bridge);
	virtual ~CApp();

	CArgsBase& argsBase() const { return *m_args; }
	CAppBridge& bridgeBase() const { return *m_bridge; }

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
	CAppBridge* m_bridge;
};

#define BYE "\nTry `%s --help' for more information."

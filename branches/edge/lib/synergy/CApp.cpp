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

#include "CApp.h"
#include "CLog.h"
#include "Version.h"
#include "ProtocolTypes.h"
#include "CArch.h"
#include "XBase.h"
#include "XArch.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#include <iostream>
#include <stdio.h>

CApp* CApp::s_instance = nullptr;

CApp::CApp(CArgsBase* args) :
m_args(args),
m_bye(&exit),
s_taskBarReceiver(NULL),
s_suspended(false)
{
	assert(s_instance == nullptr);
	s_instance = this;
}

CApp::~CApp()
{
	delete m_args;
}

CApp::CArgsBase::CArgsBase() :
m_daemon(true),
m_backend(false),
m_restartable(true),
m_noHooks(false),
m_pname(NULL),
m_logFilter(NULL),
m_logFile(NULL),
m_display(NULL)
{
}

CApp::CArgsBase::~CArgsBase()
{
}

bool
CApp::isArg(
	int argi, int argc, const char* const* argv,
	const char* name1, const char* name2,
	int minRequiredParameters)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
			// match.  check args left.
			if (argi + minRequiredParameters >= argc) {
				LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
					argsBase().m_pname, argv[argi], argsBase().m_pname));
				m_bye(kExitArgs);
			}
			return true;
	}

	// no match
	return false;
}

bool
CApp::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (ARCH->parseArg(argc, argv, i)) {
		// handled by platform util
		return true;
	}
	
	else if (isArg(i, argc, argv, "-d", "--debug", 1)) {
		// change logging level
		argsBase().m_logFilter = argv[++i];
	}

	else if (isArg(i, argc, argv, "-l", "--log", 1)) {
		argsBase().m_logFile = argv[++i];
	}

	else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
		// not a daemon
		argsBase().m_daemon = false;
	}

	else if (isArg(i, argc, argv, NULL, "--daemon")) {
		// daemonize
		argsBase().m_daemon = true;
	}

	else if (isArg(i, argc, argv, "-n", "--name", 1)) {
		// save screen name
		argsBase().m_name = argv[++i];
	}

	else if (isArg(i, argc, argv, "-1", "--no-restart")) {
		// don't try to restart
		argsBase().m_restartable = false;
	}

	else if (isArg(i, argc, argv, NULL, "--restart")) {
		// try to restart
		argsBase().m_restartable = true;
	}

	else if (isArg(i, argc, argv, "-z", NULL)) {
		argsBase().m_backend = true;
	}

	else if (isArg(i, argc, argv, NULL, "--no-hooks")) {
		argsBase().m_noHooks = true;
	}

	else if (isArg(i, argc, argv, "-h", "--help")) {
		help();
		m_bye(kExitSuccess);
	}

	else if (isArg(i, argc, argv, NULL, "--version")) {
		version();
		m_bye(kExitSuccess);
	}

	else {
		// option not supported here
		return false;
	}

	return true;
}

void
CApp::parseArgs(int argc, const char* const* argv, int& i)
{
	// about these use of assert() here:
	// previously an /analyze warning was displayed if we only used assert and
	// did not return on failure. however, this warning does not appear to show
	// any more (could be because new compiler args have been added).
	// the asserts are programmer benefit only; the os should never pass 0 args,
	// because the first is always the binary name. the only way assert would 
	// evaluate to true, is if this parse function were implemented incorrectly,
	// which is unlikely because it's old code and has a specific use.
	// we should avoid using anything other than assert here, because it will
	// look like important code, which it's not really.
	assert(argsBase().m_pname != NULL);
	assert(argv != NULL);
	assert(argc >= 1);

	// set defaults
	argsBase().m_name = ARCH->getHostName();

	// parse options
	for (i = 1; i < argc; ++i) {

		if (parseArg(argc, argv, i)) {
			continue;
		}

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
				argsBase().m_pname, argv[i], argsBase().m_pname));
			m_bye(kExitArgs);
		}

		else {
			// this and remaining arguments are not options
			break;
		}
	}

	// increase default filter level for daemon.  the user must
	// explicitly request another level for a daemon.
	if (argsBase().m_daemon && argsBase().m_logFilter == NULL) {
		argsBase().m_logFilter = "NOTE";
	}
}

void
CApp::version()
{
	char buffer[500];
	sprintf(
		buffer,
		"%s %s, protocol version %d.%d\n%s",
		argsBase().m_pname,
		kVersion,
		kProtocolMajorVersion,
		kProtocolMinorVersion,
		kCopyright
		);

	std::cout << buffer << std::endl;
}

int
CApp::run(int argc, char** argv, CreateTaskBarReceiverFunc createTaskBarReceiver)
{
#if SYSAPI_WIN32
	// record window instance for tray icon, etc
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

	CArch arch;

	// install application in to arch
	ARCH->adoptApp(this);

	// create an instance of log
	CLOG;

	int result;
	try {
		result = ARCH->run(argc, argv, createTaskBarReceiver);
	}
	catch (XBase& e) {
		LOG((CLOG_CRIT "Uncaught exception: %s\n", e.what()));
		result = kExitFailed;
	}
	catch (XArch& e) {
		LOG((CLOG_CRIT "Initialization failed: %s" BYE, e.what().c_str(), argsBase().m_pname));
		result = kExitFailed;
	}
	catch (std::exception& e) {
		LOG((CLOG_CRIT "Uncaught exception: %s\n", e.what()));
		result = kExitFailed;
	}
	catch (...) {
		LOG((CLOG_CRIT "Uncaught exception: <unknown exception>\n"));
		result = kExitFailed;
	}

	delete CLOG;

	// not sure i like what's going on here; m_bye could call exit, but it also does
	// some other stuff - if we don't return then we get compiler warning (and it's 
	// not good practice anyway), but the return will never get hit.
	m_bye(result);
	return result;
}

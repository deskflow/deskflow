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

#include "CClientApp.h"
#include "CLog.h"
#include "CArch.h"
#include "XSocket.h"
#include "Version.h"
#include "ProtocolTypes.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#if WINAPI_MSWINDOWS
#include "CMSWindowsAppUtil.h"
#endif

#include <iostream>

CClientApp::CClientApp(CAppUtil* app) :
CApp(new CArgs(), app)
{
}

CClientApp::~CClientApp()
{
}

CClientApp::CArgs::CArgs() :
m_pname(NULL),
m_backend(false),
m_restartable(true),
m_daemon(true),
m_yscroll(0),
m_logFilter(NULL),
m_display(NULL),
m_serverAddress(NULL),
m_logFile(NULL)
{
}

CClientApp::CArgs::~CArgs()
{
}

bool
CClientApp::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (isArg(i, argc, argv, "-d", "--debug", 1)) {
		// change logging level
		args().m_logFilter = argv[++i];
	}

	else if (isArg(i, argc, argv, "-n", "--name", 1)) {
		// save screen name
		args().m_name = argv[++i];
	}

	else if (isArg(i, argc, argv, NULL, "--camp")) {
		// ignore -- included for backwards compatibility
	}

	else if (isArg(i, argc, argv, NULL, "--no-camp")) {
		// ignore -- included for backwards compatibility
	}

	else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
		// not a daemon
		args().m_daemon = false;
	}

	else if (isArg(i, argc, argv, NULL, "--daemon")) {
		// daemonize
		args().m_daemon = true;
	}

#if WINAPI_XWINDOWS
	else if (isArg(i, argc, argv, "-display", "--display", 1)) {
		// use alternative display
		args().m_display = argv[++i];
	}
#endif

	else if (isArg(i, argc, argv, NULL, "--yscroll", 1)) {
		// define scroll 
		args().m_yscroll = atoi(argv[++i]);
	}

	else if (isArg(i, argc, argv, "-l", "--log", 1)) {
		args().m_logFile = argv[++i];
	}

	else if (isArg(i, argc, argv, "-1", "--no-restart")) {
		// don't try to restart
		args().m_restartable = false;
	}

	else if (isArg(i, argc, argv, NULL, "--restart")) {
		// try to restart
		args().m_restartable = true;
	}

	else if (isArg(i, argc, argv, "-z", NULL)) {
		args().m_backend = true;
	}

	else if (isArg(i, argc, argv, "-h", "--help")) {
		help();
		m_bye(kExitSuccess);
	}

	else if (isArg(i, argc, argv, NULL, "--version")) {
		version();
		m_bye(kExitSuccess);
	}

#if WINAPI_MSWINDOWS
	else if (isArg(i, argc, argv, NULL, "--service")) {

		// HACK: assume instance is an ms windows app, and call service
		// arg handler.
		// TODO: use inheritance model to fix this.
		((CMSWindowsAppUtil&)utilBase()).handleServiceArg(argv[++i]);
	}
#endif

	else if (isArg(i, argc, argv, "--", NULL)) {
		// remaining arguments are not options
		++i;
		return false;
	}

	else if (argv[i][0] == '-') {
		LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
			args().m_pname, argv[i], args().m_pname));
		m_bye(kExitArgs);
	}

	else {
		// this and remaining arguments are not options
		return false;
	}

	// argument was valid
	return true;
}

void
CClientApp::parse(int argc, const char* const* argv)
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
	assert(args().m_pname != NULL);
	assert(argv != NULL);
	assert(argc >= 1);

	// set defaults
	args().m_name = ARCH->getHostName();

	// parse options
	int i;
	for (i = 1; i < argc; ++i) {
		if (!parseArg(argc, argv, i)) {
			break;
		}
	}

	// exactly one non-option argument (server-address)
	if (i == argc) {
		LOG((CLOG_PRINT "%s: a server address or name is required" BYE,
			args().m_pname, args().m_pname));
		m_bye(kExitArgs);
	}
	if (i + 1 != argc) {
		LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
			args().m_pname, argv[i], args().m_pname));
		m_bye(kExitArgs);
	}

	// save server address
	try {
		*args().m_serverAddress = CNetworkAddress(argv[i], kDefaultPort);
		args().m_serverAddress->resolve();
	}
	catch (XSocketAddress& e) {
		// allow an address that we can't look up if we're restartable.
		// we'll try to resolve the address each time we connect to the
		// server.  a bad port will never get better.  patch by Brent
		// Priddy.
		if (!args().m_restartable || e.getError() == XSocketAddress::kBadPort) {
			LOG((CLOG_PRINT "%s: %s" BYE,
				args().m_pname, e.what(), args().m_pname));
			m_bye(kExitFailed);
		}
	}

	// increase default filter level for daemon.  the user must
	// explicitly request another level for a daemon.
	if (args().m_daemon && args().m_logFilter == NULL) {
#if SYSAPI_WIN32
		if (CArchMiscWindows::isWindows95Family()) {
			// windows 95 has no place for logging so avoid showing
			// the log console window.
			args().m_logFilter = "FATAL";
		}
		else
#endif
		{
			args().m_logFilter = "NOTE";
		}
	}

	// set log filter
	if (!CLOG->setFilter(args().m_logFilter)) {
		LOG((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
			args().m_pname, args().m_logFilter, args().m_pname));
		m_bye(kExitArgs);
	}

	// identify system
	LOG((CLOG_INFO "%s Client on %s %s", kAppVersion, ARCH->getOSName().c_str(), ARCH->getPlatformName().c_str()));

#ifdef WIN32
#ifdef _AMD64_
	LOG((CLOG_WARN "This is an experimental x64 build of %s. Use it at your own risk.", kApplication));
#endif
#endif

	if (CLOG->getFilter() > CLOG->getConsoleMaxLevel()) {
		if (args().m_logFile == NULL) {
			LOG((CLOG_WARN "log messages above %s are NOT sent to console (use file logging)", 
				CLOG->getFilterName(CLOG->getConsoleMaxLevel())));
		}
	}
}

void
CClientApp::version()
{
	char buffer[500];
	sprintf(
		buffer,
		"%s %s, protocol version %d.%d\n%s",
		args().m_pname,
		kVersion,
		kProtocolMajorVersion,
		kProtocolMinorVersion,
		kCopyright
		);

	std::cout << buffer << std::endl;
}

void
CClientApp::help()
{
#if WINAPI_XWINDOWS
#  define USAGE_DISPLAY_ARG		\
	" [--display <display>]"
#  define USAGE_DISPLAY_INFO	\
	"      --display <display>  connect to the X server at <display>\n"
#else
#  define USAGE_DISPLAY_ARG
#  define USAGE_DISPLAY_INFO
#endif

	char buffer[2000];
	sprintf(
		buffer,
		"Usage: %s"
		" [--daemon|--no-daemon]"
		" [--debug <level>]"
		USAGE_DISPLAY_ARG
		" [--name <screen-name>]"
		" [--yscroll <delta>]"
		" [--restart|--no-restart]"
		" <server-address>"
		"\n\n"
		"Start the synergy mouse/keyboard sharing server.\n"
		"\n"
		"  -d, --debug <level>      filter out log messages with priorty below level.\n"
		"                           level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n"
		"                           DEBUG, DEBUG1, DEBUG2.\n"
		USAGE_DISPLAY_INFO
		"  -f, --no-daemon          run the client in the foreground.\n"
		"*     --daemon             run the client as a daemon.\n"
		"  -n, --name <screen-name> use screen-name instead the hostname to identify\n"
		"                           ourself to the server.\n"
		"      --yscroll <delta>    defines the vertical scrolling delta, which is\n"
		"                           120 by default.\n"
		"  -1, --no-restart         do not try to restart the client if it fails for\n"
		"                           some reason.\n"
		"*     --restart            restart the client automatically if it fails.\n"
		"  -l  --log <file>         write log messages to file.\n"
		"  -h, --help               display this help and exit.\n"
		"      --version            display version information and exit.\n"
		"\n"
		"* marks defaults.\n"
		"\n"
		"The server address is of the form: [<hostname>][:<port>].  The hostname\n"
		"must be the address or hostname of the server.  The port overrides the\n"
		"default port, %d.\n"
		"\n"
		"Where log messages go depends on the platform and whether or not the\n"
		"client is running as a daemon.",
		args().m_pname, kDefaultPort
		);

	std::cout << buffer << std::endl;
}

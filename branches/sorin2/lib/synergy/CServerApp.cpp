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

#include "CServerApp.h"
#include "CLog.h"
#include "CArch.h"
#include "XSocket.h"
#include "Version.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#include <iostream>
#include <stdio.h>

CServerApp::CServerApp(CAppUtil* util) :
CApp(new CArgs(), util)
{
}

CServerApp::~CServerApp()
{
}

CServerApp::CArgs::CArgs() :
m_synergyAddress(NULL),
m_config(NULL)
{
}

CServerApp::CArgs::~CArgs()
{
}

bool
CServerApp::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (CApp::parseArg(argc, argv, i)) {
		// found common arg
		return true;
	}

	else if (isArg(i, argc, argv, "-a", "--address", 1)) {
		// save listen address
		try {
			*args().m_synergyAddress = CNetworkAddress(argv[i + 1],
				kDefaultPort);
			args().m_synergyAddress->resolve();
		}
		catch (XSocketAddress& e) {
			LOG((CLOG_PRINT "%s: %s" BYE,
				args().m_pname, e.what(), args().m_pname));
			m_bye(kExitArgs);
		}
		++i;
	}

	else if (isArg(i, argc, argv, "-c", "--config", 1)) {
		// save configuration file path
		args().m_configFile = argv[++i];
	}

	else {
		// this and remaining arguments are not options
		return false;
	}

	// argument was valid
	return true;
}

void
CServerApp::parse(int argc, const char* const* argv)
{
	// asserts values, sets defaults, and parses args
	int i;
	CApp::parse(argc, argv, i);

	// no non-option arguments are allowed
	if (i != argc) {
		LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
			args().m_pname, argv[i], args().m_pname));
		m_bye(kExitArgs);
	}

#if SYSAPI_WIN32
	// if user wants to run as daemon, but process not launched from service launcher...
	if (args().m_daemon && !CArchMiscWindows::wasLaunchedAsService()) {
		LOG((CLOG_ERR "cannot launch as daemon if process not started through "
			"service host (use '--service start' argument instead)"));
		m_bye(kExitArgs);
	}
#endif

	// set log filter
	if (!CLOG->setFilter(args().m_logFilter)) {
		LOG((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
			args().m_pname, args().m_logFilter, args().m_pname));
		m_bye(kExitArgs);
	}

	// identify system
	LOG((CLOG_INFO "%s Server on %s %s", kAppVersion, ARCH->getOSName().c_str(), ARCH->getPlatformName().c_str()));

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
CServerApp::help()
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

#if SYSAPI_WIN32

#  define PLATFORM_ARGS														\
	" [--daemon|--no-daemon]"
#  define PLATFORM_DESC
#  define PLATFORM_EXTRA													\
	"At least one command line argument is required.  If you don't otherwise\n"	\
	"need an argument use `--daemon'.\n"										\
	"\n"

#else

#  define PLATFORM_ARGS														\
	" [--daemon|--no-daemon]"
#  define PLATFORM_DESC
#  define PLATFORM_EXTRA

#endif

	char buffer[2000];
	sprintf(
		buffer,
		"Usage: %s"
		" [--address <address>]"
		" [--config <pathname>]"
		" [--debug <level>]"
		USAGE_DISPLAY_ARG
		" [--name <screen-name>]"
		" [--restart|--no-restart]"
		PLATFORM_ARGS
		"\n\n"
		"Start the synergy mouse/keyboard sharing server.\n"
		"\n"
		"  -a, --address <address>  listen for clients on the given address.\n"
		"  -c, --config <pathname>  use the named configuration file instead.\n"
		"  -d, --debug <level>      filter out log messages with priorty below level.\n"
		"                           level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n"
		"                           DEBUG, DEBUG1, DEBUG2.\n"
		USAGE_DISPLAY_INFO
		"  -f, --no-daemon          run the server in the foreground.\n"
		"*     --daemon             run the server as a daemon.\n"
		"  -n, --name <screen-name> use screen-name instead the hostname to identify\n"
		"                           this screen in the configuration.\n"
		"  -1, --no-restart         do not try to restart the server if it fails for\n"
		"                           some reason.\n"
		"*     --restart            restart the server automatically if it fails.\n"
		"  -l  --log <file>         write log messages to file.\n"
		PLATFORM_DESC
		"  -h, --help               display this help and exit.\n"
		"      --version            display version information and exit.\n"
		"\n"
		"* marks defaults.\n"
		"\n"
		PLATFORM_EXTRA
		"The argument for --address is of the form: [<hostname>][:<port>].  The\n"
		"hostname must be the address or hostname of an interface on the system.\n"
		"The default is to listen on all interfaces.  The port overrides the\n"
		"default port, %d.\n"
		"\n"
		"If no configuration file pathname is provided then the first of the\n"
		"following to load successfully sets the configuration:\n"
		"  %s\n"
		"  %s\n"
		"If no configuration file can be loaded then the configuration uses its\n"
		"defaults with just the server screen.\n"
		"\n"
		"Where log messages go depends on the platform and whether or not the\n"
		"server is running as a daemon.",
		args().m_pname, kDefaultPort,
		ARCH->concatPath(ARCH->getUserDirectory(), USR_CONFIG_NAME).c_str(),
		ARCH->concatPath(ARCH->getSystemDirectory(), SYS_CONFIG_NAME).c_str()
		);

	std::cout << buffer << std::endl;
}

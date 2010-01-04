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
#include "CMSWindowsApp.h"
#include "CArchMiscWindows.h"

#include <iostream>

CServerApp::CArgs* CServerApp::CArgs::s_instance = NULL;

CServerApp::CServerApp()
{
}

CServerApp::~CServerApp()
{
}

CServerApp::CArgs::CArgs() :
m_pname(NULL),
m_backend(false),
m_restartable(true),
m_daemon(true),
m_configFile(),
m_logFilter(NULL),
m_logFile(NULL),
m_display(NULL),
m_synergyAddress(NULL),
m_config(NULL)
{
	s_instance = this; 
}

CServerApp::CArgs::~CArgs()
{
	s_instance = NULL;
}

bool
CServerApp::isArg(
	int argi, int argc, const char* const* argv,
	const char* name1, const char* name2,
	int minRequiredParameters)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
			// match.  check args left.
			if (argi + minRequiredParameters >= argc) {
				LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
					ARG->m_pname, argv[argi], ARG->m_pname));
				m_bye(kExitArgs);
			}
			return true;
	}

	// no match
	return false;
}

void
CServerApp::parse(int argc, const char* const* argv)
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
	assert(ARG->m_pname != NULL);
	assert(argv != NULL);
	assert(argc >= 1);

	// set defaults
	ARG->m_name = ARCH->getHostName();

	// parse options
	int i = 1;
	for (; i < argc; ++i) {
		if (isArg(i, argc, argv, "-d", "--debug", 1)) {
			// change logging level
			ARG->m_logFilter = argv[++i];
		}

		else if (isArg(i, argc, argv, "-a", "--address", 1)) {
			// save listen address
			try {
				*ARG->m_synergyAddress = CNetworkAddress(argv[i + 1],
					kDefaultPort);
				ARG->m_synergyAddress->resolve();
			}
			catch (XSocketAddress& e) {
				LOG((CLOG_PRINT "%s: %s" BYE,
					ARG->m_pname, e.what(), ARG->m_pname));
				m_bye(kExitArgs);
			}
			++i;
		}

		else if (isArg(i, argc, argv, "-n", "--name", 1)) {
			// save screen name
			ARG->m_name = argv[++i];
		}

		else if (isArg(i, argc, argv, "-c", "--config", 1)) {
			// save configuration file path
			ARG->m_configFile = argv[++i];
		}

#if WINAPI_XWINDOWS
		else if (isArg(i, argc, argv, "-display", "--display", 1)) {
			// use alternative display
			ARG->m_display = argv[++i];
		}
#endif

		else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
			// not a daemon
			ARG->m_daemon = false;
		}

		else if (isArg(i, argc, argv, NULL, "--daemon")) {
			// daemonize
			ARG->m_daemon = true;
		}
		else if (isArg(i, argc, argv, "-l", "--log", 1)) {
			// logging to file
			ARG->m_logFile = argv[++i];
		}

		else if (isArg(i, argc, argv, "-1", "--no-restart")) {
			// don't try to restart
			ARG->m_restartable = false;
		}

		else if (isArg(i, argc, argv, NULL, "--restart")) {
			// try to restart
			ARG->m_restartable = true;
		}

		else if (isArg(i, argc, argv, "-z", NULL)) {
			ARG->m_backend = true;
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
			((CMSWindowsApp*)this)->handleServiceArg(argv[++i]);
		}
#endif

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
				ARG->m_pname, argv[i], ARG->m_pname));
			m_bye(kExitArgs);
		}

		else {
			// this and remaining arguments are not options
			break;
		}
	}

	// no non-option arguments are allowed
	if (i != argc) {
		LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
			ARG->m_pname, argv[i], ARG->m_pname));
		m_bye(kExitArgs);
	}

	// increase default filter level for daemon.  the user must
	// explicitly request another level for a daemon.
	if (ARG->m_daemon && ARG->m_logFilter == NULL) {
#if SYSAPI_WIN32
		if (CArchMiscWindows::isWindows95Family()) {
			// windows 95 has no place for logging so avoid showing
			// the log console window.
			ARG->m_logFilter = "FATAL";
		}
		else
#endif
		{
			ARG->m_logFilter = "NOTE";
		}
	}

#if SYSAPI_WIN32
	// if user wants to run as daemon, but process not launched from service launcher...
	if (ARG->m_daemon && !CArchMiscWindows::wasLaunchedAsService()) {
		LOG((CLOG_ERR "cannot launch as daemon if process not started through "
			"service host (use '--service start' argument instead)"));
		m_bye(kExitArgs);
	}
#endif

	// set log filter
	if (!CLOG->setFilter(ARG->m_logFilter)) {
		LOG((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
			ARG->m_pname, ARG->m_logFilter, ARG->m_pname));
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
		if (ARG->m_logFile == NULL) {
			LOG((CLOG_WARN "log messages above %s are NOT sent to console (use file logging)", 
				CLOG->getFilterName(CLOG->getConsoleMaxLevel())));
		}
	}
}

void
CServerApp::version()
{
	char buffer[500];
	sprintf(
		buffer,
		"%s %s, protocol version %d.%d\n%s",
		ARG->m_pname,
		kVersion,
		kProtocolMajorVersion,
		kProtocolMinorVersion,
		kCopyright
		);

	std::cout << buffer << std::endl;
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
		ARG->m_pname, kDefaultPort,
		ARCH->concatPath(ARCH->getUserDirectory(), USR_CONFIG_NAME).c_str(),
		ARCH->concatPath(ARCH->getSystemDirectory(), SYS_CONFIG_NAME).c_str()
		);

	std::cout << buffer << std::endl;
}

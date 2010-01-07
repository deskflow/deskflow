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
#include "IEventQueue.h"
#include "CServer.h"
#include "CClientListener.h"
#include "CClientProxy.h"
#include "TMethodEventJob.h"
#include "CServerTaskBarReceiver.h"
#include "CPrimaryClient.h"
#include "CScreen.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#include <iostream>
#include <stdio.h>
#include <fstream>

#define ARG (&args())

CServerApp::CServerApp(CAppUtil* util) :
CApp(new CArgs(), util),
s_server(NULL),
s_reloadConfigEvent(CEvent::kUnknown),
s_forceReconnectEvent(CEvent::kUnknown),
s_resetServerEvent(CEvent::kUnknown),
s_serverState(kUninitialized),
s_serverScreen(NULL),
s_primaryClient(NULL),
s_listener(NULL),
s_taskBarReceiver(NULL),
s_suspended(false),
s_timer(NULL)
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
		// option not supported here
		return false;
	}

	// argument was valid
	return true;
}

void
CServerApp::parseArgs(int argc, const char* const* argv)
{
	// asserts values, sets defaults, and parses args
	int i;
	CApp::parseArgs(argc, argv, i);

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

void
CServerApp::reloadSignalHandler(CArch::ESignal, void*)
{
	EVENTQUEUE->addEvent(CEvent(getReloadConfigEvent(),
		IEventQueue::getSystemTarget()));
}

void
CServerApp::reloadConfig(const CEvent&, void*)
{
	LOG((CLOG_DEBUG "reload configuration"));
	if (loadConfig(ARG->m_configFile)) {
		if (s_server != NULL) {
			s_server->setConfig(*ARG->m_config);
		}
		LOG((CLOG_NOTE "reloaded configuration"));
	}
}

void
CServerApp::loadConfig()
{
	bool loaded = false;

	// load the config file, if specified
	if (!ARG->m_configFile.empty()) {
		loaded = loadConfig(ARG->m_configFile);
	}

	// load the default configuration if no explicit file given
	else {
		// get the user's home directory
		CString path = ARCH->getUserDirectory();
		if (!path.empty()) {
			// complete path
			path = ARCH->concatPath(path, USR_CONFIG_NAME);

			// now try loading the user's configuration
			if (loadConfig(path)) {
				loaded            = true;
				ARG->m_configFile = path;
			}
		}
		if (!loaded) {
			// try the system-wide config file
			path = ARCH->getSystemDirectory();
			if (!path.empty()) {
				path = ARCH->concatPath(path, SYS_CONFIG_NAME);
				if (loadConfig(path)) {
					loaded            = true;
					ARG->m_configFile = path;
				}
			}
		}
	}

	if (!loaded) {
		LOG((CLOG_PRINT "%s: no configuration available", ARG->m_pname));
		m_bye(kExitConfig);
	}
}

bool
CServerApp::loadConfig(const CString& pathname)
{
	try {
		// load configuration
		LOG((CLOG_DEBUG "opening configuration \"%s\"", pathname.c_str()));
		std::ifstream configStream(pathname.c_str());
		if (!configStream.is_open()) {
			// report failure to open configuration as a debug message
			// since we try several paths and we expect some to be
			// missing.
			LOG((CLOG_DEBUG "cannot open configuration \"%s\"",
				pathname.c_str()));
			return false;
		}
		configStream >> *ARG->m_config;
		LOG((CLOG_DEBUG "configuration read successfully"));
		return true;
	}
	catch (XConfigRead& e) {
		// report error in configuration file
		LOG((CLOG_ERR "cannot read configuration \"%s\": %s",
			pathname.c_str(), e.what()));
	}
	return false;
}

CEvent::Type 
CServerApp::getReloadConfigEvent()
{
	return CEvent::registerTypeOnce(s_reloadConfigEvent, "reloadConfig");
}

void 
CServerApp::forceReconnect(const CEvent&, void*)
{
	if (s_server != NULL) {
		s_server->disconnect();
	}
}

CEvent::Type 
CServerApp::getForceReconnectEvent()
{
	return CEvent::registerTypeOnce(s_forceReconnectEvent, "forceReconnect");
}

CEvent::Type
CServerApp::getResetServerEvent()
{
	return CEvent::registerTypeOnce(s_resetServerEvent, "resetServer");
}

void 
CServerApp::handleClientConnected(const CEvent&, void* vlistener)
{
	CClientListener* listener = reinterpret_cast<CClientListener*>(vlistener);
	CClientProxy* client = listener->getNextClient();
	if (client != NULL) {
		s_server->adoptClient(client);
		updateStatus();
	}
}

void
CServerApp::handleClientsDisconnected(const CEvent&, void*)
{
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}

void
CServerApp::closeServer(CServer* server)
{
	if (server == NULL) {
		return;
	}

	// tell all clients to disconnect
	server->disconnect();

	// wait for clients to disconnect for up to timeout seconds
	double timeout = 3.0;
	CEventQueueTimer* timer = EVENTQUEUE->newOneShotTimer(timeout, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, timer,
		new TMethodEventJob<CServerApp>(this, &CServerApp::handleClientsDisconnected));
	EVENTQUEUE->adoptHandler(CServer::getDisconnectedEvent(), server,
		new TMethodEventJob<CServerApp>(this, &CServerApp::handleClientsDisconnected));
	CEvent event;
	EVENTQUEUE->getEvent(event);
	while (event.getType() != CEvent::kQuit) {
		EVENTQUEUE->dispatchEvent(event);
		CEvent::deleteData(event);
		EVENTQUEUE->getEvent(event);
	}
	EVENTQUEUE->removeHandler(CEvent::kTimer, timer);
	EVENTQUEUE->deleteTimer(timer);
	EVENTQUEUE->removeHandler(CServer::getDisconnectedEvent(), server);

	// done with server
	delete server;
}

void 
CServerApp::stopRetryTimer()
{
	if (s_timer != NULL) {
		EVENTQUEUE->deleteTimer(s_timer);
		EVENTQUEUE->removeHandler(CEvent::kTimer, NULL);
		s_timer = NULL;
	}
}

void
CServerApp::updateStatus()
{
	s_taskBarReceiver->updateStatus(s_server, "");
}

void 
CServerApp::closeClientListener(CClientListener* listen)
{
	if (listen != NULL) {
		EVENTQUEUE->removeHandler(CClientListener::getConnectedEvent(), listen);
		delete listen;
	}
}

void 
CServerApp::stopServer()
{
	if (s_serverState == kStarted) {
		closeClientListener(s_listener);
		closeServer(s_server);
		s_server      = NULL;
		s_listener    = NULL;
		s_serverState = kInitialized;
	}
	else if (s_serverState == kStarting) {
		stopRetryTimer();
		s_serverState = kInitialized;
	}
	assert(s_server == NULL);
	assert(s_listener == NULL);
}

void
CServerApp::closePrimaryClient(CPrimaryClient* primaryClient)
{
	delete primaryClient;
}

void 
CServerApp::closeServerScreen(CScreen* screen)
{
	if (screen != NULL) {
		EVENTQUEUE->removeHandler(IScreen::getErrorEvent(),
			screen->getEventTarget());
		EVENTQUEUE->removeHandler(IScreen::getSuspendEvent(),
			screen->getEventTarget());
		EVENTQUEUE->removeHandler(IScreen::getResumeEvent(),
			screen->getEventTarget());
		delete screen;
	}
}

void CServerApp::cleanupServer()
{
	stopServer();
	if (s_serverState == kInitialized) {
		closePrimaryClient(s_primaryClient);
		closeServerScreen(s_serverScreen);
		s_primaryClient = NULL;
		s_serverScreen  = NULL;
		s_serverState   = kUninitialized;
	}
	else if (s_serverState == kInitializing ||
		s_serverState == kInitializingToStart) {
			stopRetryTimer();
			s_serverState = kUninitialized;
	}
	assert(s_primaryClient == NULL);
	assert(s_serverScreen == NULL);
	assert(s_serverState == kUninitialized);
}
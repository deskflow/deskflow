/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include "CSocketMultiplexer.h"
#include "CEventQueue.h"
#include "LogOutputters.h"
#include "CFunctionEventJob.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#if WINAPI_MSWINDOWS
#include "CMSWindowsScreen.h"
#elif WINAPI_XWINDOWS
#include "CXWindowsScreen.h"
#elif WINAPI_CARBON
#include "COSXScreen.h"
#endif

#include <iostream>
#include <stdio.h>
#include <fstream>
#include "XScreen.h"
#include "CTCPSocketFactory.h"

CEvent::Type CServerApp::s_reloadConfigEvent = CEvent::kUnknown;

CServerApp::CServerApp(CreateTaskBarReceiverFunc createTaskBarReceiver) :
CApp(createTaskBarReceiver, new CArgs()),
s_server(NULL),
s_forceReconnectEvent(CEvent::kUnknown),
s_resetServerEvent(CEvent::kUnknown),
s_serverState(kUninitialized),
s_serverScreen(NULL),
s_primaryClient(NULL),
s_listener(NULL),
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

	// set log filter
	if (!CLOG->setFilter(args().m_logFilter)) {
		LOG((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
			args().m_pname, args().m_logFilter, args().m_pname));
		m_bye(kExitArgs);
	}

	// identify system
	LOG((CLOG_INFO "%s Server on %s %s", kAppVersion, ARCH->getOSName().c_str(), ARCH->getPlatformName().c_str()));

	loggingFilterWarning();
}

void
CServerApp::help()
{
	// window api args (windows/x-windows/carbon)
#if WINAPI_XWINDOWS
#  define WINAPI_ARGS \
	" [--display <display>] [--no-xinitthreads]"
#  define WINAPI_INFO \
	"      --display <display>  connect to the X server at <display>\n" \
	"      --no-xinitthreads    do not call XInitThreads()\n"
#else
#  define WINAPI_ARGS
#  define WINAPI_INFO
#endif

	char buffer[2000];
	sprintf(
		buffer,
		"Usage: %s"
		" [--address <address>]"
		" [--config <pathname>]"
		WINAPI_ARGS
		HELP_SYS_ARGS
		HELP_COMMON_ARGS
		"\n\n"
		"Start the synergy mouse/keyboard sharing server.\n"
		"\n"
		"  -a, --address <address>  listen for clients on the given address.\n"
		"  -c, --config <pathname>  use the named configuration file instead.\n"
		HELP_COMMON_INFO_1
		WINAPI_INFO
		HELP_SYS_INFO
		HELP_COMMON_INFO_2
		"\n"
		"* marks defaults.\n"
		"\n"
		"The argument for --address is of the form: [<hostname>][:<port>].  The\n"
		"hostname must be the address or hostname of an interface on the system.\n"
		"The default is to listen on all interfaces.  The port overrides the\n"
		"default port, %d.\n"
		"\n"
		"If no configuration file pathname is provided then the first of the\n"
		"following to load successfully sets the configuration:\n"
		"  %s\n"
		"  %s\n",
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
	if (loadConfig(args().m_configFile)) {
		if (s_server != NULL) {
			s_server->setConfig(*args().m_config);
		}
		LOG((CLOG_NOTE "reloaded configuration"));
	}
}

void
CServerApp::loadConfig()
{
	bool loaded = false;

	// load the config file, if specified
	if (!args().m_configFile.empty()) {
		loaded = loadConfig(args().m_configFile);
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
				args().m_configFile = path;
			}
		}
		if (!loaded) {
			// try the system-wide config file
			path = ARCH->getSystemDirectory();
			if (!path.empty()) {
				path = ARCH->concatPath(path, SYS_CONFIG_NAME);
				if (loadConfig(path)) {
					loaded            = true;
					args().m_configFile = path;
				}
			}
		}
	}

	if (!loaded) {
		LOG((CLOG_PRINT "%s: no configuration available", args().m_pname));
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
		configStream >> *args().m_config;
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
	return EVENTQUEUE->registerTypeOnce(s_reloadConfigEvent, "reloadConfig");
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
	return EVENTQUEUE->registerTypeOnce(s_forceReconnectEvent, "forceReconnect");
}

CEvent::Type
CServerApp::getResetServerEvent()
{
	return EVENTQUEUE->registerTypeOnce(s_resetServerEvent, "resetServer");
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
	updateStatus("");
}

void CServerApp::updateStatus( const CString& msg )
{
	if (m_taskBarReceiver)
	{
		m_taskBarReceiver->updateStatus(s_server, msg);
	}
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

void
CServerApp::retryHandler(const CEvent&, void*)
{
	// discard old timer
	assert(s_timer != NULL);
	stopRetryTimer();

	// try initializing/starting the server again
	switch (s_serverState) {
	case kUninitialized:
	case kInitialized:
	case kStarted:
		assert(0 && "bad internal server state");
		break;

	case kInitializing:
		LOG((CLOG_DEBUG1 "retry server initialization"));
		s_serverState = kUninitialized;
		if (!initServer()) {
			EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
		}
		break;

	case kInitializingToStart:
		LOG((CLOG_DEBUG1 "retry server initialization"));
		s_serverState = kUninitialized;
		if (!initServer()) {
			EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
		}
		else if (s_serverState == kInitialized) {
			LOG((CLOG_DEBUG1 "starting server"));
			if (!startServer()) {
				EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
			}
		}
		break;

	case kStarting:
		LOG((CLOG_DEBUG1 "retry starting server"));
		s_serverState = kInitialized;
		if (!startServer()) {
			EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
		}
		break;
	}
}

bool CServerApp::initServer()
{
	// skip if already initialized or initializing
	if (s_serverState != kUninitialized) {
		return true;
	}

	double retryTime;
	CScreen* serverScreen         = NULL;
	CPrimaryClient* primaryClient = NULL;
	try {
		CString name    = args().m_config->getCanonicalName(args().m_name);
		serverScreen    = openServerScreen();
		primaryClient   = openPrimaryClient(name, serverScreen);
		s_serverScreen  = serverScreen;
		s_primaryClient = primaryClient;
		s_serverState   = kInitialized;
		updateStatus();
		return true;
	}
	catch (XScreenUnavailable& e) {
		LOG((CLOG_WARN "cannot open primary screen: %s", e.what()));
		closePrimaryClient(primaryClient);
		closeServerScreen(serverScreen);
		updateStatus(CString("cannot open primary screen: ") + e.what());
		retryTime = e.getRetryTime();
	}
	catch (XScreenOpenFailure& e) {
		LOG((CLOG_CRIT "cannot open primary screen: %s", e.what()));
		closePrimaryClient(primaryClient);
		closeServerScreen(serverScreen);
		return false;
	}
	catch (XBase& e) {
		LOG((CLOG_CRIT "failed to start server: %s", e.what()));
		closePrimaryClient(primaryClient);
		closeServerScreen(serverScreen);
		return false;
	}

	if (args().m_restartable) {
		// install a timer and handler to retry later
		assert(s_timer == NULL);
		LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
		s_timer = EVENTQUEUE->newOneShotTimer(retryTime, NULL);
		EVENTQUEUE->adoptHandler(CEvent::kTimer, s_timer,
			new TMethodEventJob<CServerApp>(this, &CServerApp::retryHandler));
		s_serverState = kInitializing;
		return true;
	}
	else {
		// don't try again
		return false;
	}
}

CScreen* CServerApp::openServerScreen()
{
	CScreen* screen = createScreen();
	EVENTQUEUE->adoptHandler(IScreen::getErrorEvent(),
		screen->getEventTarget(),
		new TMethodEventJob<CServerApp>(
		this, &CServerApp::handleScreenError));
	EVENTQUEUE->adoptHandler(IScreen::getSuspendEvent(),
		screen->getEventTarget(),
		new TMethodEventJob<CServerApp>(
		this, &CServerApp::handleSuspend));
	EVENTQUEUE->adoptHandler(IScreen::getResumeEvent(),
		screen->getEventTarget(),
		new TMethodEventJob<CServerApp>(
		this, &CServerApp::handleResume));
	return screen;
}

bool 
CServerApp::startServer()
{
	// skip if already started or starting
	if (s_serverState == kStarting || s_serverState == kStarted) {
		return true;
	}

	// initialize if necessary
	if (s_serverState != kInitialized) {
		if (!initServer()) {
			// hard initialization failure
			return false;
		}
		if (s_serverState == kInitializing) {
			// not ready to start
			s_serverState = kInitializingToStart;
			return true;
		}
		assert(s_serverState == kInitialized);
	}

	double retryTime;
	CClientListener* listener = NULL;
	try {
		listener   = openClientListener(args().m_config->getSynergyAddress());
		s_server   = openServer(*args().m_config, s_primaryClient);
		listener->setServer(s_server);
		s_listener = listener;
		updateStatus();
		LOG((CLOG_NOTE "started server"));
		s_serverState = kStarted;
		return true;
	}
	catch (XSocketAddressInUse& e) {
		LOG((CLOG_WARN "cannot listen for clients: %s", e.what()));
		closeClientListener(listener);
		updateStatus(CString("cannot listen for clients: ") + e.what());
		retryTime = 10.0;
	}
	catch (XBase& e) {
		LOG((CLOG_CRIT "failed to start server: %s", e.what()));
		closeClientListener(listener);
		return false;
	}

	if (args().m_restartable) {
		// install a timer and handler to retry later
		assert(s_timer == NULL);
		LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
		s_timer = EVENTQUEUE->newOneShotTimer(retryTime, NULL);
		EVENTQUEUE->adoptHandler(CEvent::kTimer, s_timer,
			new TMethodEventJob<CServerApp>(this, &CServerApp::retryHandler));
		s_serverState = kStarting;
		return true;
	}
	else {
		// don't try again
		return false;
	}
}

CScreen* 
CServerApp::createScreen()
{
#if WINAPI_MSWINDOWS
	return new CScreen(new CMSWindowsScreen(true, args().m_noHooks, args().m_gameDevice));
#elif WINAPI_XWINDOWS
	return new CScreen(new CXWindowsScreen(
		args().m_display, true, args().m_disableXInitThreads, 0, *EVENTQUEUE));
#elif WINAPI_CARBON
	return new CScreen(new COSXScreen(true));
#endif
}

CPrimaryClient* 
CServerApp::openPrimaryClient(const CString& name, CScreen* screen)
{
	LOG((CLOG_DEBUG1 "creating primary screen"));
	return new CPrimaryClient(name, screen);

}

void
CServerApp::handleScreenError(const CEvent&, void*)
{
	LOG((CLOG_CRIT "error on screen"));
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}

void 
CServerApp::handleSuspend(const CEvent&, void*)
{
	if (!m_suspended) {
		LOG((CLOG_INFO "suspend"));
		stopServer();
		m_suspended = true;
	}
}

void 
CServerApp::handleResume(const CEvent&, void*)
{
	if (m_suspended) {
		LOG((CLOG_INFO "resume"));
		startServer();
		m_suspended = false;
	}
}

CClientListener*
CServerApp::openClientListener(const CNetworkAddress& address)
{
	CClientListener* listen =
		new CClientListener(address, new CTCPSocketFactory, NULL);
	EVENTQUEUE->adoptHandler(CClientListener::getConnectedEvent(), listen,
		new TMethodEventJob<CServerApp>(
		this, &CServerApp::handleClientConnected, listen));
	return listen;
}

CServer* 
CServerApp::openServer(const CConfig& config, CPrimaryClient* primaryClient)
{
	CServer* server = new CServer(config, primaryClient, s_serverScreen);

	try {
		EVENTQUEUE->adoptHandler(
			CServer::getDisconnectedEvent(), server,
			new TMethodEventJob<CServerApp>(this, &CServerApp::handleNoClients));

	} catch (std::bad_alloc &ba) {
		delete server;
		throw ba;
	}

	return server;
}

void CServerApp::handleNoClients( const CEvent&, void* )
{
	updateStatus();
}

int CServerApp::mainLoop()
{
	// create socket multiplexer.  this must happen after daemonization
	// on unix because threads evaporate across a fork().
	CSocketMultiplexer multiplexer;

	// create the event queue
	CEventQueue eventQueue;

	// if configuration has no screens then add this system
	// as the default
	if (args().m_config->begin() == args().m_config->end()) {
		args().m_config->addScreen(args().m_name);
	}

	// set the contact address, if provided, in the config.
	// otherwise, if the config doesn't have an address, use
	// the default.
	if (args().m_synergyAddress->isValid()) {
		args().m_config->setSynergyAddress(*args().m_synergyAddress);
	}
	else if (!args().m_config->getSynergyAddress().isValid()) {
		args().m_config->setSynergyAddress(CNetworkAddress(kDefaultPort));
	}

	// canonicalize the primary screen name
	CString primaryName = args().m_config->getCanonicalName(args().m_name);
	if (primaryName.empty()) {
		LOG((CLOG_CRIT "unknown screen name `%s'", args().m_name.c_str()));
		return kExitFailed;
	}

	// start server, etc
	ARCH->util().startNode();

	// handle hangup signal by reloading the server's configuration
	ARCH->setSignalHandler(CArch::kHANGUP, &reloadSignalHandler, NULL);
	EVENTQUEUE->adoptHandler(getReloadConfigEvent(),
		IEventQueue::getSystemTarget(),
		new TMethodEventJob<CServerApp>(this, &CServerApp::reloadConfig));

	// handle force reconnect event by disconnecting clients.  they'll
	// reconnect automatically.
	EVENTQUEUE->adoptHandler(getForceReconnectEvent(),
		IEventQueue::getSystemTarget(),
		new TMethodEventJob<CServerApp>(this, &CServerApp::forceReconnect));

	// to work around the sticky meta keys problem, we'll give users
	// the option to reset the state of synergys
	EVENTQUEUE->adoptHandler(getResetServerEvent(),
		IEventQueue::getSystemTarget(),
		new TMethodEventJob<CServerApp>(this, &CServerApp::resetServer));

	// run event loop.  if startServer() failed we're supposed to retry
	// later.  the timer installed by startServer() will take care of
	// that.
	CEvent event;
	DAEMON_RUNNING(true);
	EVENTQUEUE->getEvent(event);
	while (event.getType() != CEvent::kQuit) {
		EVENTQUEUE->dispatchEvent(event);
		CEvent::deleteData(event);
		EVENTQUEUE->getEvent(event);
	}
	DAEMON_RUNNING(false);

	// close down
	LOG((CLOG_DEBUG1 "stopping server"));
	EVENTQUEUE->removeHandler(getForceReconnectEvent(),
		IEventQueue::getSystemTarget());
	EVENTQUEUE->removeHandler(getReloadConfigEvent(),
		IEventQueue::getSystemTarget());
	cleanupServer();
	updateStatus();
	LOG((CLOG_NOTE "stopped server"));

	return kExitSuccess;
}

void CServerApp::resetServer(const CEvent&, void*)
{
	LOG((CLOG_DEBUG1 "resetting server"));
	stopServer();
	cleanupServer();
	startServer();
}

int 
CServerApp::runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup)
{
	// general initialization
	args().m_synergyAddress = new CNetworkAddress;
	args().m_config         = new CConfig;
	args().m_pname          = ARCH->getBasename(argv[0]);

	// install caller's output filter
	if (outputter != NULL) {
		CLOG->insert(outputter);
	}

	// run
	int result = startup(argc, argv);

	if (m_taskBarReceiver)
	{
		// done with task bar receiver
		delete m_taskBarReceiver;
	}

	delete args().m_config;
	delete args().m_synergyAddress;
	return result;
}

int daemonMainLoopStatic(int argc, const char** argv) {
	return CServerApp::instance().daemonMainLoop(argc, argv);
}

int 
CServerApp::standardStartup(int argc, char** argv)
{
	initApp(argc, argv);

	// daemonize if requested
	if (args().m_daemon) {
		return ARCH->daemonize(daemonName(), daemonMainLoopStatic);
	}
	else {
		return mainLoop();
	}
}

int 
CServerApp::foregroundStartup(int argc, char** argv)
{
	initApp(argc, argv);

	// never daemonize
	return mainLoop();
}

static
int 
mainLoopStatic() 
{
	return CServerApp::instance().mainLoop();
}

const char* 
CServerApp::daemonName() const
{
#if SYSAPI_WIN32
	return "Synergy Server";
#elif SYSAPI_UNIX
	return "synergys";
#endif
}

const char* 
CServerApp::daemonInfo() const
{
#if SYSAPI_WIN32
	return "Shares this computers mouse and keyboard with other computers.";
#elif SYSAPI_UNIX
	return "";
#endif
}

void
CServerApp::startNode()
{
	// start the server.  if this return false then we've failed and
	// we shouldn't retry.
	LOG((CLOG_DEBUG1 "starting server"));
	if (!startServer()) {
		m_bye(kExitFailed);
	}
}

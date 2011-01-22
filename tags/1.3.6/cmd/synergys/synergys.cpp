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

#include "CClientListener.h"
#include "CClientProxy.h"
#include "CConfig.h"
#include "CPrimaryClient.h"
#include "CServer.h"
#include "CScreen.h"
#include "ProtocolTypes.h"
#include "Version.h"
#include "XScreen.h"
#include "CSocketMultiplexer.h"
#include "CTCPSocketFactory.h"
#include "XSocket.h"
#include "CThread.h"
#include "CEventQueue.h"
#include "CFunctionEventJob.h"
#include "CLog.h"
#include "CString.h"
#include "CStringUtil.h"
#include "LogOutputters.h"
#include "CArch.h"
#include "XArch.h"
#include "stdfstream.h"
#include <cstring>

#define DAEMON_RUNNING(running_)
#if WINAPI_MSWINDOWS
#include "CArchMiscWindows.h"
#include "CMSWindowsScreen.h"
#include "CMSWindowsUtil.h"
#include "CMSWindowsServerTaskBarReceiver.h"
#include "resource.h"
#undef DAEMON_RUNNING
#define DAEMON_RUNNING(running_) CArchMiscWindows::daemonRunning(running_)
#elif WINAPI_XWINDOWS
#include "CXWindowsScreen.h"
#include "CXWindowsServerTaskBarReceiver.h"
#elif WINAPI_CARBON
#include "COSXScreen.h"
#include "COSXServerTaskBarReceiver.h"
#endif

// platform dependent name of a daemon
#if SYSAPI_WIN32
#define DAEMON_NAME "Synergy Server"
#elif SYSAPI_UNIX
#define DAEMON_NAME "synergys"
#endif

// configuration file name
#if SYSAPI_WIN32
#define USR_CONFIG_NAME "synergy.sgc"
#define SYS_CONFIG_NAME "synergy.sgc"
#elif SYSAPI_UNIX
#define USR_CONFIG_NAME ".synergy.conf"
#define SYS_CONFIG_NAME "synergy.conf"
#endif

typedef int (*StartupFunc)(int, char**);
static void parse(int argc, const char* const* argv);
static bool loadConfig(const CString& pathname);
static void loadConfig();

//
// program arguments
//

#define ARG CArgs::s_instance

class CArgs {
public:
	CArgs() :
		m_pname(NULL),
		m_backend(false),
		m_restartable(true),
		m_daemon(true),
		m_configFile(),
		m_logFilter(NULL),
		m_logFile(NULL),
		m_display(NULL),
		m_synergyAddress(NULL),
		m_config(NULL),
		m_disableXInitThreads(false)
		{ s_instance = this; }
	~CArgs() { s_instance = NULL; }

public:
	static CArgs*		s_instance;
	const char* 		m_pname;
	bool				m_backend;
	bool				m_restartable;
	bool				m_daemon;
	CString		 		m_configFile;
	const char* 		m_logFilter;
	const char*			m_logFile;
	const char*			m_display;
	CString 			m_name;
	CNetworkAddress*	m_synergyAddress;
	CConfig*			m_config;
	bool				m_disableXInitThreads;
};

CArgs*					CArgs::s_instance = NULL;


//
// platform dependent factories
//

static
CScreen*
createScreen()
{
#if WINAPI_MSWINDOWS
	return new CScreen(new CMSWindowsScreen(true));
#elif WINAPI_XWINDOWS
	return new CScreen(new CXWindowsScreen(
		ARG->m_display, true, ARG->m_disableXInitThreads));
#elif WINAPI_CARBON
	return new CScreen(new COSXScreen(true));
#endif
}

static
CServerTaskBarReceiver*
createTaskBarReceiver(const CBufferedLogOutputter* logBuffer)
{
#if WINAPI_MSWINDOWS
	return new CMSWindowsServerTaskBarReceiver(
							CMSWindowsScreen::getInstance(), logBuffer);
#elif WINAPI_XWINDOWS
	return new CXWindowsServerTaskBarReceiver(logBuffer);
#elif WINAPI_CARBON
	return new COSXServerTaskBarReceiver(logBuffer);
#endif
}


//
// platform independent main
//

enum EServerState {
	kUninitialized,
	kInitializing,
	kInitializingToStart,
	kInitialized,
	kStarting,
	kStarted
};

static EServerState				s_serverState         = kUninitialized;
static CServer*					s_server              = NULL;
static CScreen*					s_serverScreen        = NULL;
static CPrimaryClient*			s_primaryClient       = NULL;
static CClientListener*			s_listener            = NULL;
static CServerTaskBarReceiver*	s_taskBarReceiver     = NULL;
static CEvent::Type				s_reloadConfigEvent   = CEvent::kUnknown;
static CEvent::Type				s_forceReconnectEvent = CEvent::kUnknown;
static CEvent::Type				s_resetServerEvent	  = CEvent::kUnknown;
static bool						s_suspended           = false;
static CEventQueueTimer*		s_timer               = NULL;

CEvent::Type
getReloadConfigEvent()
{
	return CEvent::registerTypeOnce(s_reloadConfigEvent, "reloadConfig");
}

CEvent::Type
getForceReconnectEvent()
{
	return CEvent::registerTypeOnce(s_forceReconnectEvent, "forceReconnect");
}

CEvent::Type
getResetServerEvent()
{
	return CEvent::registerTypeOnce(s_resetServerEvent, "resetServer");
}

static
void
updateStatus()
{
	s_taskBarReceiver->updateStatus(s_server, "");
}

static
void
updateStatus(const CString& msg)
{
	s_taskBarReceiver->updateStatus(s_server, msg);
}

static
void
handleClientConnected(const CEvent&, void* vlistener)
{
	CClientListener* listener = reinterpret_cast<CClientListener*>(vlistener);
	CClientProxy* client = listener->getNextClient();
	if (client != NULL) {
		s_server->adoptClient(client);
		updateStatus();
	}
}

static
CClientListener*
openClientListener(const CNetworkAddress& address)
{
	CClientListener* listen =
		new CClientListener(address, new CTCPSocketFactory, NULL);
	EVENTQUEUE->adoptHandler(CClientListener::getConnectedEvent(), listen,
							new CFunctionEventJob(
								&handleClientConnected, listen));
	return listen;
}

static
void
closeClientListener(CClientListener* listen)
{
	if (listen != NULL) {
		EVENTQUEUE->removeHandler(CClientListener::getConnectedEvent(), listen);
		delete listen;
	}
}

static
void
handleScreenError(const CEvent&, void*)
{
	LOG((CLOG_CRIT "error on screen"));
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}


static void handleSuspend(const CEvent& event, void*);
static void handleResume(const CEvent& event, void*);

static
CScreen*
openServerScreen()
{
	CScreen* screen = createScreen();
	EVENTQUEUE->adoptHandler(IScreen::getErrorEvent(),
							screen->getEventTarget(),
							new CFunctionEventJob(
								&handleScreenError));
	EVENTQUEUE->adoptHandler(IScreen::getSuspendEvent(),
							screen->getEventTarget(),
							new CFunctionEventJob(
								&handleSuspend));
	EVENTQUEUE->adoptHandler(IScreen::getResumeEvent(),
							screen->getEventTarget(),
							new CFunctionEventJob(
								&handleResume));
	return screen;
}

static
void
closeServerScreen(CScreen* screen)
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

static
CPrimaryClient*
openPrimaryClient(const CString& name, CScreen* screen)
{
	LOG((CLOG_DEBUG1 "creating primary screen"));
	return new CPrimaryClient(name, screen);
}

static
void
closePrimaryClient(CPrimaryClient* primaryClient)
{
	delete primaryClient;
}

static
void
handleNoClients(const CEvent&, void*)
{
	updateStatus();
}

static
void
handleClientsDisconnected(const CEvent&, void*)
{
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}

static
CServer*
openServer(const CConfig& config, CPrimaryClient* primaryClient)
{
	CServer* server = new CServer(config, primaryClient);
	EVENTQUEUE->adoptHandler(CServer::getDisconnectedEvent(), server,
						new CFunctionEventJob(handleNoClients));
	return server;
}

static
void
closeServer(CServer* server)
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
						new CFunctionEventJob(handleClientsDisconnected));
	EVENTQUEUE->adoptHandler(CServer::getDisconnectedEvent(), server,
						new CFunctionEventJob(handleClientsDisconnected));
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

static bool initServer();
static bool startServer();

static
void
stopRetryTimer()
{
	if (s_timer != NULL) {
		EVENTQUEUE->deleteTimer(s_timer);
		EVENTQUEUE->removeHandler(CEvent::kTimer, NULL);
		s_timer = NULL;
	}
}

static
void
retryHandler(const CEvent&, void*)
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

static
bool
initServer()
{
	// skip if already initialized or initializing
	if (s_serverState != kUninitialized) {
		return true;
	}

	double retryTime;
	CScreen* serverScreen         = NULL;
	CPrimaryClient* primaryClient = NULL;
	try {
		CString name    = ARG->m_config->getCanonicalName(ARG->m_name);
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
	
	if (ARG->m_restartable) {
		// install a timer and handler to retry later
		assert(s_timer == NULL);
		LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
		s_timer = EVENTQUEUE->newOneShotTimer(retryTime, NULL);
		EVENTQUEUE->adoptHandler(CEvent::kTimer, s_timer,
							new CFunctionEventJob(&retryHandler, NULL));
		s_serverState = kInitializing;
		return true;
	}
	else {
		// don't try again
		return false;
	}
}

static
bool
startServer()
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
		listener   = openClientListener(ARG->m_config->getSynergyAddress());
		s_server   = openServer(*ARG->m_config, s_primaryClient);
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

	if (ARG->m_restartable) {
		// install a timer and handler to retry later
		assert(s_timer == NULL);
		LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
		s_timer = EVENTQUEUE->newOneShotTimer(retryTime, NULL);
		EVENTQUEUE->adoptHandler(CEvent::kTimer, s_timer,
							new CFunctionEventJob(&retryHandler, NULL));
		s_serverState = kStarting;
		return true;
	}
	else {
		// don't try again
		return false;
	}
}

static
void
stopServer()
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

static
void
cleanupServer()
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

static
void
handleSuspend(const CEvent&, void*)
{
	if (!s_suspended) {
		LOG((CLOG_INFO "suspend"));
		stopServer();
		s_suspended = true;
	}
}

static
void
handleResume(const CEvent&, void*)
{
	if (s_suspended) {
		LOG((CLOG_INFO "resume"));
		startServer();
		s_suspended = false;
	}
}

static
void
reloadSignalHandler(CArch::ESignal, void*)
{
	EVENTQUEUE->addEvent(CEvent(getReloadConfigEvent(),
							IEventQueue::getSystemTarget()));
}

static
void
reloadConfig(const CEvent&, void*)
{
	LOG((CLOG_DEBUG "reload configuration"));
	if (loadConfig(ARG->m_configFile)) {
		if (s_server != NULL) {
			s_server->setConfig(*ARG->m_config);
		}
		LOG((CLOG_NOTE "reloaded configuration"));
	}
}

static
void
forceReconnect(const CEvent&, void*)
{
	if (s_server != NULL) {
		s_server->disconnect();
	}
}

// simply stops and starts the server in order to try and
// work around issues like the sticky meta keys problem, etc
static
void 
resetServer(const CEvent&, void*)
{
	LOG((CLOG_DEBUG1 "resetting server"));
	stopServer();
	cleanupServer();
	startServer();
}

static
int
mainLoop()
{
	// create socket multiplexer.  this must happen after daemonization
	// on unix because threads evaporate across a fork().
	CSocketMultiplexer multiplexer;

	// create the event queue
	CEventQueue eventQueue;

	// logging to files
	CFileLogOutputter* fileLog = NULL;

	if (ARG->m_logFile != NULL) {
		fileLog = new CFileLogOutputter(ARG->m_logFile);

		CLOG->insert(fileLog);

		LOG((CLOG_DEBUG1 "Logging to file (%s) enabled", ARG->m_logFile));
	}

	// if configuration has no screens then add this system
	// as the default
	if (ARG->m_config->begin() == ARG->m_config->end()) {
		ARG->m_config->addScreen(ARG->m_name);
	}

	// set the contact address, if provided, in the config.
	// otherwise, if the config doesn't have an address, use
	// the default.
	if (ARG->m_synergyAddress->isValid()) {
		ARG->m_config->setSynergyAddress(*ARG->m_synergyAddress);
	}
	else if (!ARG->m_config->getSynergyAddress().isValid()) {
		ARG->m_config->setSynergyAddress(CNetworkAddress(kDefaultPort));
	}

	// canonicalize the primary screen name
	CString primaryName = ARG->m_config->getCanonicalName(ARG->m_name);
	if (primaryName.empty()) {
		LOG((CLOG_CRIT "unknown screen name `%s'", ARG->m_name.c_str()));
		return kExitFailed;
	}

	// start the server.  if this return false then we've failed and
	// we shouldn't retry.
	LOG((CLOG_DEBUG1 "starting server"));
	if (!startServer()) {
		return kExitFailed;
	}

	// handle hangup signal by reloading the server's configuration
	ARCH->setSignalHandler(CArch::kHANGUP, &reloadSignalHandler, NULL);
	EVENTQUEUE->adoptHandler(getReloadConfigEvent(),
							IEventQueue::getSystemTarget(),
							new CFunctionEventJob(&reloadConfig));

	// handle force reconnect event by disconnecting clients.  they'll
	// reconnect automatically.
	EVENTQUEUE->adoptHandler(getForceReconnectEvent(),
							IEventQueue::getSystemTarget(),
							new CFunctionEventJob(&forceReconnect));

	// to work around the sticky meta keys problem, we'll give users
	// the option to reset the state of synergys
	EVENTQUEUE->adoptHandler(getResetServerEvent(),
							IEventQueue::getSystemTarget(),
							new CFunctionEventJob(&resetServer));

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

	if (fileLog) {
		CLOG->remove(fileLog);
		delete fileLog;		
	}

	return kExitSuccess;
}

static
int
daemonMainLoop(int, const char**)
{
#if SYSAPI_WIN32
	CSystemLogger sysLogger(DAEMON_NAME, false);
#else
	CSystemLogger sysLogger(DAEMON_NAME, true);
#endif
	return mainLoop();
}

static
int
standardStartup(int argc, char** argv)
{

	// parse command line
	parse(argc, argv);

	// load configuration
	loadConfig();

	// daemonize if requested
	if (ARG->m_daemon) {
		return ARCH->daemonize(DAEMON_NAME, &daemonMainLoop);
	}
	else {
		return mainLoop();
	}
}

static
int
run(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup)
{
	// general initialization
	ARG->m_synergyAddress = new CNetworkAddress;
	ARG->m_config         = new CConfig;
	ARG->m_pname          = ARCH->getBasename(argv[0]);

	// install caller's output filter
	if (outputter != NULL) {
		CLOG->insert(outputter);
	}

	// save log messages
	CBufferedLogOutputter logBuffer(1000);
	CLOG->insert(&logBuffer, true);

	// make the task bar receiver.  the user can control this app
	// through the task bar.
	s_taskBarReceiver = createTaskBarReceiver(&logBuffer);

	// run
	int result = startup(argc, argv);

	// done with task bar receiver
	delete s_taskBarReceiver;

	// done with log buffer
	CLOG->remove(&logBuffer);

	delete ARG->m_config;
	delete ARG->m_synergyAddress;
	return result;
}


//
// command line parsing
//

#define BYE "\nTry `%s --help' for more information."

static void				(*bye)(int) = &exit;

static
void
version()
{
	LOG((CLOG_PRINT
"%s %s, protocol version %d.%d\n"
"%s",
								ARG->m_pname,
								kVersion,
								kProtocolMajorVersion,
								kProtocolMinorVersion,
								kCopyright));
}

static
void
help()
{
#if WINAPI_XWINDOWS
#  define USAGE_DISPLAY_ARG		\
" [--display <display>] [--no-xinitthreads]"
#  define USAGE_DISPLAY_INFO	\
"      --display <display>  connect to the X server at <display>\n" \
"      --no-xinitthreads    do not call XInitThreads()\n"
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

	LOG((CLOG_PRINT
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
"  -d, --debug <level>      filter out log messages with priority below level.\n"
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
								ARG->m_pname,
								kDefaultPort,
								ARCH->concatPath(
									ARCH->getUserDirectory(),
									USR_CONFIG_NAME).c_str(),
								ARCH->concatPath(
									ARCH->getSystemDirectory(),
									SYS_CONFIG_NAME).c_str()));
}

static
bool
isArg(int argi, int argc, const char* const* argv,
				const char* name1, const char* name2,
				int minRequiredParameters = 0)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
		// match.  check args left.
		if (argi + minRequiredParameters >= argc) {
			LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
								ARG->m_pname, argv[argi], ARG->m_pname));
			bye(kExitArgs);
		}
		return true;
	}

	// no match
	return false;
}

static
void
parse(int argc, const char* const* argv)
{
	assert(ARG->m_pname != NULL);
	assert(argv       != NULL);
	assert(argc       >= 1);

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
				bye(kExitArgs);
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
			bye(kExitSuccess);
		}

		else if (isArg(i, argc, argv, NULL, "--version")) {
			version();
			bye(kExitSuccess);
		}

		else if (isArg(i, argc, argv, NULL, "--no-xinitthreads")) {
			ARG->m_disableXInitThreads = true;
		}

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								ARG->m_pname, argv[i], ARG->m_pname));
			bye(kExitArgs);
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
		bye(kExitArgs);
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

	// set log filter
	if (!CLOG->setFilter(ARG->m_logFilter)) {
		LOG((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
								ARG->m_pname, ARG->m_logFilter, ARG->m_pname));
		bye(kExitArgs);
	}

	// identify system
	LOG((CLOG_INFO "%s Server on %s %s", kAppVersion, ARCH->getOSName().c_str(), ARCH->getPlatformName().c_str()));

#ifdef WIN32
#ifdef _AMD64_
	LOG((CLOG_WARN "This is an experimental x64 build of %s. Use it at your own risk.", kApplication));
#endif
#endif
}

static
bool
loadConfig(const CString& pathname)
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

static
void
loadConfig()
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
		bye(kExitConfig);
	}
}


//
// platform dependent entry points
//

#if SYSAPI_WIN32

static bool				s_hasImportantLogMessages = false;

//
// CMessageBoxOutputter
//
// This class writes severe log messages to a message box
//

class CMessageBoxOutputter : public ILogOutputter {
public:
	CMessageBoxOutputter() { }
	virtual ~CMessageBoxOutputter() { }

	// ILogOutputter overrides
	virtual void		open(const char*) { }
	virtual void		close() { }
	virtual void		show(bool) { }
	virtual bool		write(ELevel level, const char* message);
	virtual const char*	getNewline() const { return ""; }
};

bool
CMessageBoxOutputter::write(ELevel level, const char* message)
{
	// note any important messages the user may need to know about
	if (level <= CLog::kWARNING) {
		s_hasImportantLogMessages = true;
	}

	// FATAL and PRINT messages get a dialog box if not running as
	// backend.  if we're running as a backend the user will have
	// a chance to see the messages when we exit.
	if (!ARG->m_backend && level <= CLog::kFATAL) {
		MessageBox(NULL, message, ARG->m_pname, MB_OK | MB_ICONWARNING);
		return false;
	}
	else {
		return true;
	}
}

static
void
byeThrow(int x)
{
	CArchMiscWindows::daemonFailed(x);
}

static
int
daemonNTMainLoop(int argc, const char** argv)
{
	parse(argc, argv);
	ARG->m_backend = false;
	loadConfig();
	return CArchMiscWindows::runDaemon(mainLoop);
}

static
int
daemonNTStartup(int, char**)
{
	CSystemLogger sysLogger(DAEMON_NAME, false);
	bye = &byeThrow;
	return ARCH->daemonize(DAEMON_NAME, &daemonNTMainLoop);
}

static
int
foregroundStartup(int argc, char** argv)
{

	// parse command line
	parse(argc, argv);

	// load configuration
	loadConfig();

	// never daemonize
	return mainLoop();
}

static
void
showError(HINSTANCE instance, const char* title, UINT id, const char* arg)
{
	CString fmt = CMSWindowsUtil::getString(instance, id);
	CString msg = CStringUtil::format(fmt.c_str(), arg);
	MessageBox(NULL, msg.c_str(), title, MB_OK | MB_ICONWARNING);
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	try {
		CArchMiscWindows::setIcons((HICON)LoadImage(instance,
									MAKEINTRESOURCE(IDI_SYNERGY),
									IMAGE_ICON,
									32, 32, LR_SHARED),
									(HICON)LoadImage(instance,
									MAKEINTRESOURCE(IDI_SYNERGY),
									IMAGE_ICON,
									16, 16, LR_SHARED));
		CArch arch(instance);
		CMSWindowsScreen::init(instance);
		CLOG;
		CThread::getCurrentThread().setPriority(-14);
		CArgs args;

		// set title on log window
		ARCH->openConsole((CString(kAppVersion) + " " + "Server").c_str());

		// windows NT family starts services using no command line options.
		// since i'm not sure how to tell the difference between that and
		// a user providing no options we'll assume that if there are no
		// arguments and we're on NT then we're being invoked as a service.
		// users on NT can use `--daemon' or `--no-daemon' to force us out
		// of the service code path.
		StartupFunc startup = &standardStartup;
		if (!CArchMiscWindows::isWindows95Family()) {
			if (__argc <= 1) {
				startup = &daemonNTStartup;
			}
			else {
				startup = &foregroundStartup;
			}
		}

		// send PRINT and FATAL output to a message box
		int result = run(__argc, __argv, new CMessageBoxOutputter, startup);

		// let user examine any messages if we're running as a backend
		// by putting up a dialog box before exiting.
		if (args.m_backend && s_hasImportantLogMessages) {
			showError(instance, args.m_pname, IDS_FAILED, "");
		}

		delete CLOG;
		return result;
	}
	catch (XBase& e) {
		showError(instance, __argv[0], IDS_UNCAUGHT_EXCEPTION, e.what());
		//throw;
	}
	catch (XArch& e) {
		showError(instance, __argv[0], IDS_INIT_FAILED, e.what().c_str());
	}
	catch (...) {
		showError(instance, __argv[0], IDS_UNCAUGHT_EXCEPTION, "<unknown>");
		//throw;
	}
	return kExitFailed;
}

#elif SYSAPI_UNIX

int
main(int argc, char** argv)
{
	CArgs args;
	try {
		int result;
		CArch arch;
		CLOG;
		CArgs args;
		result = run(argc, argv, NULL, &standardStartup);
		delete CLOG;
		return result;
	}
	catch (XBase& e) {
		LOG((CLOG_CRIT "Uncaught exception: %s\n", e.what()));
		throw;
	}
	catch (XArch& e) {
		LOG((CLOG_CRIT "Initialization failed: %s" BYE, e.what().c_str()));
		return kExitFailed;
	}
	catch (...) {
		LOG((CLOG_CRIT "Uncaught exception: <unknown exception>\n"));
		throw;
	}
}

#else

#error no main() for platform

#endif

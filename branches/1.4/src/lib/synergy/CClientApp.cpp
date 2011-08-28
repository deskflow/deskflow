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

#include "CClientApp.h"
#include "CLog.h"
#include "CArch.h"
#include "XSocket.h"
#include "Version.h"
#include "ProtocolTypes.h"
#include "CString.h"
#include "CScreen.h"
#include "CEvent.h"
#include "CClient.h"
#include "CNetworkAddress.h"
#include "IArchTaskBarReceiver.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "CTCPSocketFactory.h"
#include "XScreen.h"
#include "LogOutputters.h"
#include "CSocketMultiplexer.h"
#include "CEventQueue.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#if SYSAPI_WIN32 && GAME_DEVICE_SUPPORT
#include <Windows.h>
#include "XInputHook.h"
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

#define RETRY_TIME 1.0

CClientApp::CClientApp(CreateTaskBarReceiverFunc createTaskBarReceiver) :
CApp(createTaskBarReceiver, new CArgs()),
s_client(NULL),
s_clientScreen(NULL)
{
}

CClientApp::~CClientApp()
{
}

CClientApp::CArgs::CArgs() :
m_yscroll(0),
m_serverAddress(NULL)
{
}

CClientApp::CArgs::~CArgs()
{
}

bool
CClientApp::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (CApp::parseArg(argc, argv, i)) {
		// found common arg
		return true;
	}

	else if (isArg(i, argc, argv, NULL, "--camp")) {
		// ignore -- included for backwards compatibility
	}

	else if (isArg(i, argc, argv, NULL, "--no-camp")) {
		// ignore -- included for backwards compatibility
	}

	else if (isArg(i, argc, argv, NULL, "--yscroll", 1)) {
		// define scroll 
		args().m_yscroll = atoi(argv[++i]);
	}

	else {
		// option not supported here
		return false;
	}

	// argument was valid
	return true;
}

void
CClientApp::parseArgs(int argc, const char* const* argv)
{
	// asserts values, sets defaults, and parses args
	int i;
	CApp::parseArgs(argc, argv, i);

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

	// set log filter
	if (!CLOG->setFilter(args().m_logFilter)) {
		LOG((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
			args().m_pname, args().m_logFilter, args().m_pname));
		m_bye(kExitArgs);
	}

	// identify system
	LOG((CLOG_INFO "%s Client on %s %s", kAppVersion, ARCH->getOSName().c_str(), ARCH->getPlatformName().c_str()));

	loggingFilterWarning();
}

void
CClientApp::help()
{
#if WINAPI_XWINDOWS
#  define WINAPI_ARG \
	" [--display <display>] [--no-xinitthreads]"
#  define WINAPI_INFO \
	"      --display <display>  connect to the X server at <display>\n" \
	"      --no-xinitthreads    do not call XInitThreads()\n"
#else
#  define WINAPI_ARG
#  define WINAPI_INFO
#endif

	char buffer[2000];
	sprintf(
		buffer,
		"Usage: %s"
		" [--yscroll <delta>]"
		WINAPI_ARG
		HELP_SYS_ARGS
		HELP_COMMON_ARGS
		" <server-address>"
		"\n\n"
		"Connect to a synergy mouse/keyboard sharing server.\n"
		"\n"
		HELP_COMMON_INFO_1
		WINAPI_INFO
		HELP_SYS_INFO
		"      --yscroll <delta>    defines the vertical scrolling delta, which is\n"
		"                             120 by default.\n"
		HELP_COMMON_INFO_2
		"\n"
		"* marks defaults.\n"
		"\n"
		"The server address is of the form: [<hostname>][:<port>].  The hostname\n"
		"must be the address or hostname of the server.  The port overrides the\n"
		"default port, %d.\n",
		args().m_pname, kDefaultPort
		);

	std::cout << buffer << std::endl;
}

const char*
CClientApp::daemonName() const
{
#if SYSAPI_WIN32
	return "Synergy Client";
#elif SYSAPI_UNIX
	return "synergyc";
#endif
}

const char*
CClientApp::daemonInfo() const
{
#if SYSAPI_WIN32
	return "Allows another computer to share it's keyboard and mouse with this computer.";
#elif SYSAPI_UNIX
	return "";
#endif
}

CScreen*
CClientApp::createScreen()
{
#if WINAPI_MSWINDOWS
	return new CScreen(new CMSWindowsScreen(false, args().m_noHooks, args().m_gameDevice));
#elif WINAPI_XWINDOWS
	return new CScreen(new CXWindowsScreen(
		args().m_display, false, args().m_disableXInitThreads,
		args().m_yscroll));
#elif WINAPI_CARBON
	return new CScreen(new COSXScreen(false));
#endif
}

void
CClientApp::updateStatus()
{
	updateStatus("");
}


void
CClientApp::updateStatus(const CString& msg)
{
	if (m_taskBarReceiver)
	{
		m_taskBarReceiver->updateStatus(s_client, msg);
	}
}


void
CClientApp::resetRestartTimeout()
{
	// retry time can nolonger be changed
	//s_retryTime = 0.0;
}


double
CClientApp::nextRestartTimeout()
{
	// retry at a constant rate (Issue 52)
	return RETRY_TIME;

	/*
	// choose next restart timeout.  we start with rapid retries
	// then slow down.
	if (s_retryTime < 1.0) {
	s_retryTime = 1.0;
	}
	else if (s_retryTime < 3.0) {
	s_retryTime = 3.0;
	}
	else {
	s_retryTime = 5.0;
	}
	return s_retryTime;
	*/
}


void
CClientApp::handleScreenError(const CEvent&, void*)
{
	LOG((CLOG_CRIT "error on screen"));
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}


CScreen*
CClientApp::openClientScreen()
{
	CScreen* screen = createScreen();
	EVENTQUEUE->adoptHandler(IScreen::getErrorEvent(),
		screen->getEventTarget(),
		new TMethodEventJob<CClientApp>(
		this, &CClientApp::handleScreenError));
	return screen;
}


void
CClientApp::closeClientScreen(CScreen* screen)
{
	if (screen != NULL) {
		EVENTQUEUE->removeHandler(IScreen::getErrorEvent(),
			screen->getEventTarget());
		delete screen;
	}
}


void
CClientApp::handleClientRestart(const CEvent&, void* vtimer)
{
	// discard old timer
	CEventQueueTimer* timer = reinterpret_cast<CEventQueueTimer*>(vtimer);
	EVENTQUEUE->deleteTimer(timer);
	EVENTQUEUE->removeHandler(CEvent::kTimer, timer);

	// reconnect
	startClient();
}


void
CClientApp::scheduleClientRestart(double retryTime)
{
	// install a timer and handler to retry later
	LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
	CEventQueueTimer* timer = EVENTQUEUE->newOneShotTimer(retryTime, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, timer,
		new TMethodEventJob<CClientApp>(this, &CClientApp::handleClientRestart, timer));
}


void
CClientApp::handleClientConnected(const CEvent&, void*)
{
	LOG((CLOG_NOTE "connected to server"));
	resetRestartTimeout();
	updateStatus();
}


void
CClientApp::handleClientFailed(const CEvent& e, void*)
{
	CClient::CFailInfo* info =
		reinterpret_cast<CClient::CFailInfo*>(e.getData());

	updateStatus(CString("Failed to connect to server: ") + info->m_what);
	if (!args().m_restartable || !info->m_retry) {
		LOG((CLOG_ERR "failed to connect to server: %s", info->m_what.c_str()));
		EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
	}
	else {
		LOG((CLOG_WARN "failed to connect to server: %s", info->m_what.c_str()));
		if (!m_suspended) {
			scheduleClientRestart(nextRestartTimeout());
		}
	}
	delete info;
}


void
CClientApp::handleClientDisconnected(const CEvent&, void*)
{
	LOG((CLOG_NOTE "disconnected from server"));
	if (!args().m_restartable) {
		EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
	}
	else if (!m_suspended) {
		s_client->connect();
	}
	updateStatus();
}


CClient*
CClientApp::openClient(const CString& name, const CNetworkAddress& address, CScreen* screen)
{
	CClient* client = new CClient(
		name, address, new CTCPSocketFactory, NULL, screen);

	try {
		EVENTQUEUE->adoptHandler(
			CClient::getConnectedEvent(),
			client->getEventTarget(),
			new TMethodEventJob<CClientApp>(this, &CClientApp::handleClientConnected));

		EVENTQUEUE->adoptHandler(
			CClient::getConnectionFailedEvent(),
			client->getEventTarget(),
			new TMethodEventJob<CClientApp>(this, &CClientApp::handleClientFailed));

		EVENTQUEUE->adoptHandler(
			CClient::getDisconnectedEvent(),
			client->getEventTarget(),
			new TMethodEventJob<CClientApp>(this, &CClientApp::handleClientDisconnected));

	} catch (std::bad_alloc &ba) {
		delete client;
		throw ba;
	}

	return client;
}


void
CClientApp::closeClient(CClient* client)
{
	if (client == NULL) {
		return;
	}

	EVENTQUEUE->removeHandler(CClient::getConnectedEvent(), client);
	EVENTQUEUE->removeHandler(CClient::getConnectionFailedEvent(), client);
	EVENTQUEUE->removeHandler(CClient::getDisconnectedEvent(), client);
	delete client;
}

int
CClientApp::foregroundStartup(int argc, char** argv)
{
	initApp(argc, argv);

	// never daemonize
	return mainLoop();
}

bool
CClientApp::startClient()
{
	double retryTime;
	CScreen* clientScreen = NULL;
	try {
		if (s_clientScreen == NULL) {
			clientScreen = openClientScreen();
			s_client     = openClient(args().m_name,
				*args().m_serverAddress, clientScreen);
			s_clientScreen  = clientScreen;
			LOG((CLOG_NOTE "started client"));
		}

#if SYSAPI_WIN32 && GAME_DEVICE_SUPPORT
		if (args().m_gameDevice)
		{
			// TODO: currently this is failing because we're not
			// forcing compile with the DX XInput.h (so the win
			// SDK is being used)... we need to figure out how to
			// tell cmake to prefer the DX include path.
			LOG((CLOG_DEBUG "installing xinput hook"));
			InstallXInputHook();
		}
#endif

		s_client->connect();
		updateStatus();
		return true;
	}
	catch (XScreenUnavailable& e) {
		LOG((CLOG_WARN "cannot open secondary screen: %s", e.what()));
		closeClientScreen(clientScreen);
		updateStatus(CString("Cannot open secondary screen: ") + e.what());
		retryTime = e.getRetryTime();
	}
	catch (XScreenOpenFailure& e) {
		LOG((CLOG_CRIT "cannot open secondary screen: %s", e.what()));
		closeClientScreen(clientScreen);
		return false;
	}
	catch (XBase& e) {
		LOG((CLOG_CRIT "failed to start client: %s", e.what()));
		closeClientScreen(clientScreen);
		return false;
	}

	if (args().m_restartable) {
		scheduleClientRestart(retryTime);
		return true;
	}
	else {
		// don't try again
		return false;
	}
}


void
CClientApp::stopClient()
{
#if SYSAPI_WIN32 && GAME_DEVICE_SUPPORT
	if (args().m_gameDevice)
	{
		LOG((CLOG_DEBUG "removing xinput hook"));
		RemoveXInputHook();
	}
#endif

	closeClient(s_client);
	closeClientScreen(s_clientScreen);
	s_client       = NULL;
	s_clientScreen = NULL;
}


int
CClientApp::mainLoop()
{
	// create socket multiplexer.  this must happen after daemonization
	// on unix because threads evaporate across a fork().
	CSocketMultiplexer multiplexer;

	// create the event queue
	CEventQueue eventQueue;

	// start client, etc
	ARCH->util().startNode();

	// run event loop.  if startClient() failed we're supposed to retry
	// later.  the timer installed by startClient() will take care of
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
	LOG((CLOG_DEBUG1 "stopping client"));
	stopClient();
	updateStatus();
	LOG((CLOG_NOTE "stopped client"));

	return kExitSuccess;
}

static
int
daemonMainLoopStatic(int argc, const char** argv)
{
	return CClientApp::instance().daemonMainLoop(argc, argv);
}

int
CClientApp::standardStartup(int argc, char** argv)
{
	initApp(argc, argv);

	// daemonize if requested
	if (args().m_daemon) {
		return ARCH->daemonize(daemonName(), &daemonMainLoopStatic);
	}
	else {
		return mainLoop();
	}
}

int
CClientApp::runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup)
{
	// general initialization
	args().m_serverAddress = new CNetworkAddress;
	args().m_pname         = ARCH->getBasename(argv[0]);

	// install caller's output filter
	if (outputter != NULL) {
		CLOG->insert(outputter);
	}

	int result;
	try
	{
		// run
		result = startup(argc, argv);
	}
	catch (...)
	{
		if (m_taskBarReceiver)
		{
			// done with task bar receiver
			delete m_taskBarReceiver;
		}

		delete args().m_serverAddress;

		throw;
	}

	return result;
}

void 
CClientApp::startNode()
{
	// start the client.  if this return false then we've failed and
	// we shouldn't retry.
	LOG((CLOG_DEBUG1 "starting client"));
	if (!startClient()) {
		m_bye(kExitFailed);
	}
}

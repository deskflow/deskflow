/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synergy/ClientApp.h"

#include "client/Client.h"
#include "synergy/ArgParser.h"
#include "synergy/protocol_types.h"
#include "synergy/Screen.h"
#include "synergy/XScreen.h"
#include "synergy/ClientArgs.h"
#include "net/NetworkAddress.h"
#include "net/TCPSocketFactory.h"
#include "net/SocketMultiplexer.h"
#include "net/XSocket.h"
#include "mt/Thread.h"
#include "arch/IArchTaskBarReceiver.h"
#include "arch/Arch.h"
#include "base/String.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"
#include "base/log_outputters.h"
#include "base/EventQueue.h"
#include "base/TMethodJob.h"
#include "base/Log.h"
#include "common/Version.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

#if WINAPI_MSWINDOWS
#include "platform/MSWindowsScreen.h"
#elif WINAPI_XWINDOWS
#include "platform/XWindowsScreen.h"
#elif WINAPI_CARBON
#include "platform/OSXScreen.h"
#endif

#if defined(__APPLE__)
#include "platform/OSXDragSimulator.h"
#endif

#include <iostream>
#include <stdio.h>

#define RETRY_TIME 1.0

CClientApp::CClientApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver) :
	CApp(events, createTaskBarReceiver, new CClientArgs()),
	m_client(NULL),
	m_clientScreen(NULL),
	m_serverAddress(NULL)
{
}

CClientApp::~CClientApp()
{
}

void
CClientApp::parseArgs(int argc, const char* const* argv)
{
	CArgParser argParser(this);
	bool result = argParser.parseClientArgs(args(), argc, argv);

	if (!result || args().m_shouldExit) {
		m_bye(kExitArgs);
	}
	else {
		// save server address
		if (!args().m_synergyAddress.empty()) {
			try {
				*m_serverAddress = CNetworkAddress(args().m_synergyAddress, kDefaultPort);
				m_serverAddress->resolve();
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
		}
	}
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

	LOG((CLOG_PRINT "%s", buffer));
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
	return new CScreen(new CMSWindowsScreen(
		false, args().m_noHooks, args().m_stopOnDeskSwitch, m_events), m_events);
#elif WINAPI_XWINDOWS
	return new CScreen(new CXWindowsScreen(
		args().m_display, false, args().m_disableXInitThreads,
		args().m_yscroll, m_events), m_events);
#elif WINAPI_CARBON
	return new CScreen(new COSXScreen(m_events, false), m_events);
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
		m_taskBarReceiver->updateStatus(m_client, msg);
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
	m_events->addEvent(CEvent(CEvent::kQuit));
}


CScreen*
CClientApp::openClientScreen()
{
	CScreen* screen = createScreen();
	screen->setEnableDragDrop(argsBase().m_enableDragDrop);
	m_events->adoptHandler(m_events->forIScreen().error(),
		screen->getEventTarget(),
		new TMethodEventJob<CClientApp>(
		this, &CClientApp::handleScreenError));
	return screen;
}


void
CClientApp::closeClientScreen(CScreen* screen)
{
	if (screen != NULL) {
		m_events->removeHandler(m_events->forIScreen().error(),
			screen->getEventTarget());
		delete screen;
	}
}


void
CClientApp::handleClientRestart(const CEvent&, void* vtimer)
{
	// discard old timer
	CEventQueueTimer* timer = reinterpret_cast<CEventQueueTimer*>(vtimer);
	m_events->deleteTimer(timer);
	m_events->removeHandler(CEvent::kTimer, timer);

	// reconnect
	startClient();
}


void
CClientApp::scheduleClientRestart(double retryTime)
{
	// install a timer and handler to retry later
	LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
	CEventQueueTimer* timer = m_events->newOneShotTimer(retryTime, NULL);
	m_events->adoptHandler(CEvent::kTimer, timer,
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
		m_events->addEvent(CEvent(CEvent::kQuit));
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
		m_events->addEvent(CEvent(CEvent::kQuit));
	}
	else if (!m_suspended) {
		m_client->connect();
	}
	updateStatus();
}


CClient*
CClientApp::openClient(const CString& name, const CNetworkAddress& address, CScreen* screen, const CCryptoOptions& crypto)
{
	CClient* client = new CClient(
		m_events,
		name,
		address,
		new CTCPSocketFactory(m_events, getSocketMultiplexer()),
		NULL,
		screen,
		crypto,
		args().m_enableDragDrop);

	try {
		m_events->adoptHandler(
			m_events->forCClient().connected(),
			client->getEventTarget(),
			new TMethodEventJob<CClientApp>(this, &CClientApp::handleClientConnected));

		m_events->adoptHandler(
			m_events->forCClient().connectionFailed(),
			client->getEventTarget(),
			new TMethodEventJob<CClientApp>(this, &CClientApp::handleClientFailed));

		m_events->adoptHandler(
			m_events->forCClient().disconnected(),
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

	m_events->removeHandler(m_events->forCClient().connected(), client);
	m_events->removeHandler(m_events->forCClient().connectionFailed(), client);
	m_events->removeHandler(m_events->forCClient().disconnected(), client);
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
		if (m_clientScreen == NULL) {
			clientScreen = openClientScreen();
			m_client     = openClient(args().m_name,
				*m_serverAddress, clientScreen, args().m_crypto);
			m_clientScreen  = clientScreen;
			LOG((CLOG_NOTE "started client"));
		}

		m_client->connect();

		updateStatus();
		return true;
	}
	catch (XScreenUnavailable& e) {
		LOG((CLOG_WARN "secondary screen unavailable: %s", e.what()));
		closeClientScreen(clientScreen);
		updateStatus(CString("secondary screen unavailable: ") + e.what());
		retryTime = e.getRetryTime();
	}
	catch (XScreenOpenFailure& e) {
		LOG((CLOG_CRIT "failed to start client: %s", e.what()));
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
	closeClient(m_client);
	closeClientScreen(m_clientScreen);
	m_client       = NULL;
	m_clientScreen = NULL;
}


int
CClientApp::mainLoop()
{
	// create socket multiplexer.  this must happen after daemonization
	// on unix because threads evaporate across a fork().
	CSocketMultiplexer multiplexer;
	setSocketMultiplexer(&multiplexer);

	// start client, etc
	appUtil().startNode();
	
	// init ipc client after node start, since create a new screen wipes out
	// the event queue (the screen ctors call adoptBuffer).
	if (argsBase().m_enableIpc) {
		initIpcClient();
	}

	// load all available plugins.
	ARCH->plugin().init(m_clientScreen->getEventTarget(), m_events);

	// run event loop.  if startClient() failed we're supposed to retry
	// later.  the timer installed by startClient() will take care of
	// that.
	DAEMON_RUNNING(true);
	
#if defined(MAC_OS_X_VERSION_10_7)
	
	CThread thread(
		new TMethodJob<CClientApp>(
			this, &CClientApp::runEventsLoop,
			NULL));
	
	// wait until carbon loop is ready
	COSXScreen* screen = dynamic_cast<COSXScreen*>(
		m_clientScreen->getPlatformScreen());
	screen->waitForCarbonLoop();
	
	runCocoaApp();
#else
	m_events->loop();
#endif
	
	DAEMON_RUNNING(false);

	// close down
	LOG((CLOG_DEBUG1 "stopping client"));
	stopClient();
	updateStatus();
	LOG((CLOG_NOTE "stopped client"));

	if (argsBase().m_enableIpc) {
		cleanupIpcClient();
	}

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
	m_serverAddress = new CNetworkAddress;
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

		delete m_serverAddress;

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

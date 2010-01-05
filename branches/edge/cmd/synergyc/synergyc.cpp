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

#include "CClient.h"
#include "CScreen.h"
#include "ProtocolTypes.h"
#include "Version.h"
#include "XScreen.h"
#include "CNetworkAddress.h"
#include "CSocketMultiplexer.h"
#include "CTCPSocketFactory.h"
#include "XSocket.h"
#include "CThread.h"
#include "CEventQueue.h"
#include "CFunctionEventJob.h"
#include "CFunctionJob.h"
#include "CLog.h"
#include "CString.h"
#include "CStringUtil.h"
#include "LogOutputters.h"
#include "CArch.h"
#include "XArch.h"

#include <cstring>
#include <iostream>

#define DAEMON_RUNNING(running_)
#if WINAPI_MSWINDOWS
#include "CArchMiscWindows.h"
#include "CMSWindowsScreen.h"
#include "CMSWindowsUtil.h"
#include "CMSWindowsClientTaskBarReceiver.h"
#include "resource.h"
#include <conio.h>
#include "CMSWindowsClientApp.h"
#include "CMSWindowsAppUtil.h"
#undef DAEMON_RUNNING
#define DAEMON_RUNNING(running_) CArchMiscWindows::daemonRunning(running_)
#elif WINAPI_XWINDOWS
#include "CXWindowsScreen.h"
#include "CXWindowsClientTaskBarReceiver.h"
#include "CXWindowsClientApp.h"
#include "CXWindowsAppUtil.h"
#elif WINAPI_CARBON
#include "COSXScreen.h"
#include "COSXClientTaskBarReceiver.h"
#include "COSXClientApp.h"
#include "COSXAppUtil.h"
#endif

// platform dependent name of a daemon
#if SYSAPI_WIN32
#define DAEMON_NAME "Synergy+ Client"
#define DAEMON_INFO "Allows another computer to share it's keyboard and mouse with this computer."
#elif SYSAPI_UNIX
#define DAEMON_NAME "synergyc"
#endif

typedef int (*StartupFunc)(int, char**);
static bool startClient();

#if WINAPI_MSWINDOWS
CMSWindowsClientApp app;
#elif WINAPI_XWINDOWS
CXWindowsClientApp app;
#elif WINAPI_CARBON
COSXClientApp app;
#endif

#define ARG ((CClientApp::CArgs*)&app.args())

//
// platform dependent factories
//

static
CScreen*
createScreen()
{
#if WINAPI_MSWINDOWS
	return new CScreen(new CMSWindowsScreen(false));
#elif WINAPI_XWINDOWS
	return new CScreen(new CXWindowsScreen(ARG->m_display, false, ARG->m_yscroll));
#elif WINAPI_CARBON
	return new CScreen(new COSXScreen(false));
#endif
}

static
CClientTaskBarReceiver*
createTaskBarReceiver(const CBufferedLogOutputter* logBuffer)
{
#if WINAPI_MSWINDOWS
	return new CMSWindowsClientTaskBarReceiver(
							CMSWindowsScreen::getInstance(), logBuffer);
#elif WINAPI_XWINDOWS
	return new CXWindowsClientTaskBarReceiver(logBuffer);
#elif WINAPI_CARBON
	return new COSXClientTaskBarReceiver(logBuffer);
#endif
}


//
// platform independent main
//

static CClient*					s_client          = NULL;
static CScreen*					s_clientScreen    = NULL;
static CClientTaskBarReceiver*	s_taskBarReceiver = NULL;
//static double					s_retryTime       = 0.0;
static bool						s_suspened        = false;

#define RETRY_TIME 1.0

static
void
updateStatus()
{
	s_taskBarReceiver->updateStatus(s_client, "");
}

static
void
updateStatus(const CString& msg)
{
	s_taskBarReceiver->updateStatus(s_client, msg);
}

static
void
resetRestartTimeout()
{
	// retry time can nolonger be changed
	//s_retryTime = 0.0;
}

static
double
nextRestartTimeout()
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

static
void
handleScreenError(const CEvent&, void*)
{
	LOG((CLOG_CRIT "error on screen"));
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}

static
CScreen*
openClientScreen()
{
	CScreen* screen = createScreen();
	EVENTQUEUE->adoptHandler(IScreen::getErrorEvent(),
							screen->getEventTarget(),
							new CFunctionEventJob(
								&handleScreenError));
	return screen;
}

static
void
closeClientScreen(CScreen* screen)
{
	if (screen != NULL) {
		EVENTQUEUE->removeHandler(IScreen::getErrorEvent(),
							screen->getEventTarget());
		delete screen;
	}
}

static
void
handleClientRestart(const CEvent&, void* vtimer)
{
	// discard old timer
	CEventQueueTimer* timer = reinterpret_cast<CEventQueueTimer*>(vtimer);
	EVENTQUEUE->deleteTimer(timer);
	EVENTQUEUE->removeHandler(CEvent::kTimer, timer);

	// reconnect
	startClient();
}

static
void
scheduleClientRestart(double retryTime)
{
	// install a timer and handler to retry later
	LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
	CEventQueueTimer* timer = EVENTQUEUE->newOneShotTimer(retryTime, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, timer,
							new CFunctionEventJob(&handleClientRestart, timer));
}

static
void
handleClientConnected(const CEvent&, void*)
{
	LOG((CLOG_NOTE "connected to server"));
	resetRestartTimeout();
	updateStatus();
}

static
void
handleClientFailed(const CEvent& e, void*)
{
	CClient::CFailInfo* info =
		reinterpret_cast<CClient::CFailInfo*>(e.getData());

	updateStatus(CString("Failed to connect to server: ") + info->m_what);
	if (!ARG->m_restartable || !info->m_retry) {
		LOG((CLOG_ERR "failed to connect to server: %s", info->m_what.c_str()));
		EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
	}
	else {
		LOG((CLOG_WARN "failed to connect to server: %s", info->m_what.c_str()));
		if (!s_suspened) {
			scheduleClientRestart(nextRestartTimeout());
		}
	}
	delete info;
}

static
void
handleClientDisconnected(const CEvent&, void*)
{
	LOG((CLOG_NOTE "disconnected from server"));
	if (!ARG->m_restartable) {
		EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
	}
	else if (!s_suspened) {
		s_client->connect();
	}
	updateStatus();
}

static
CClient*
openClient(const CString& name, const CNetworkAddress& address, CScreen* screen)
{
	CClient* client = new CClient(
		name, address, new CTCPSocketFactory, NULL, screen);

	try {
		EVENTQUEUE->adoptHandler(
			CClient::getConnectedEvent(),
			client->getEventTarget(),
			new CFunctionEventJob(handleClientConnected));

		EVENTQUEUE->adoptHandler(
			CClient::getConnectionFailedEvent(),
			client->getEventTarget(),
			new CFunctionEventJob(handleClientFailed));

		EVENTQUEUE->adoptHandler(
			CClient::getDisconnectedEvent(),
			client->getEventTarget(),
			new CFunctionEventJob(handleClientDisconnected));

	} catch (std::bad_alloc &ba) {
		delete client;
		throw ba;
	}

	return client;
}

static
void
closeClient(CClient* client)
{
	if (client == NULL) {
		return;
	}

	EVENTQUEUE->removeHandler(CClient::getConnectedEvent(), client);
	EVENTQUEUE->removeHandler(CClient::getConnectionFailedEvent(), client);
	EVENTQUEUE->removeHandler(CClient::getDisconnectedEvent(), client);
	delete client;
}

static
bool
startClient()
{
	double retryTime;
	CScreen* clientScreen = NULL;
	try {
		if (s_clientScreen == NULL) {
			clientScreen = openClientScreen();
			s_client     = openClient(ARG->m_name,
							*ARG->m_serverAddress, clientScreen);
			s_clientScreen  = clientScreen;
			LOG((CLOG_NOTE "started client"));
		}
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

	if (ARG->m_restartable) {
		scheduleClientRestart(retryTime);
		return true;
	}
	else {
		// don't try again
		return false;
	}
}

static
void
stopClient()
{
	closeClient(s_client);
	closeClientScreen(s_clientScreen);
	s_client       = NULL;
	s_clientScreen = NULL;
}

static
int
mainLoop()
{
	// logging to files
	CFileLogOutputter* fileLog = NULL;

	if (ARG->m_logFile != NULL) {
		fileLog = new CFileLogOutputter(ARG->m_logFile);

		CLOG->insert(fileLog);

		LOG((CLOG_DEBUG1 "Logging to file (%s) enabled", ARG->m_logFile));
	}

	// create socket multiplexer.  this must happen after daemonization
	// on unix because threads evaporate across a fork().
	CSocketMultiplexer multiplexer;

	// create the event queue
	CEventQueue eventQueue;

	// start the client.  if this return false then we've failed and
	// we shouldn't retry.
	LOG((CLOG_DEBUG1 "starting client"));
	if (!startClient()) {
		return kExitFailed;
	}

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
	if (!ARG->m_daemon) {
		ARCH->showConsole(false);
	}

	// parse command line
	app.parse(argc, argv);

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
	ARG->m_serverAddress = new CNetworkAddress;
	ARG->m_pname         = ARCH->getBasename(argv[0]);

	// install caller's output filter
	if (outputter != NULL) {
		CLOG->insert(outputter);
	}

	// save log messages
	// use heap memory because CLog deletes outputters on destruction
	CBufferedLogOutputter* logBuffer = new CBufferedLogOutputter(1000);
	CLOG->insert(logBuffer, true);

	// make the task bar receiver.  the user can control this app
	// through the task bar.
	s_taskBarReceiver = createTaskBarReceiver(logBuffer);

	// run
	int result = startup(argc, argv);

	// done with task bar receiver
	delete s_taskBarReceiver;

	delete ARG->m_serverAddress;
	return result;
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
	app.parse(argc, argv);
	ARG->m_backend = false;
	return CArchMiscWindows::runDaemon(mainLoop);
}

static
int
daemonNTStartup(int, char**)
{
	CSystemLogger sysLogger(DAEMON_NAME, false);
	app.m_bye = &byeThrow;
	return ARCH->daemonize(DAEMON_NAME, &daemonNTMainLoop);
}

static
int
foregroundStartup(int argc, char** argv)
{
	ARCH->showConsole(false);

	// parse command line
	app.parse(argc, argv);

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

int main(int argc, char** argv) {

	app.m_daemonName = DAEMON_NAME;
	app.m_daemonInfo = DAEMON_INFO;
	app.util().m_instance = GetModuleHandle(NULL);

	if (app.util().m_instance) {
		return WinMain(app.util().m_instance, NULL, GetCommandLine(), SW_SHOWNORMAL);
	} else {
		return kExitFailed;
	}
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

		StartupFunc startup;
		if (!CArchMiscWindows::isWindows95Family()) {

			// WARNING: this may break backwards computability!
			// previously, we were assuming that the process is launched from the
			// service host when no arguments were passed. if we wanted to launch
			// from console or debugger, we had to remember to pass -f which was
			// always the first pitfall for new committers. now, we are able to
			// check using the new `wasLaunchedAsService` function, which is a
			// more elegant solution.
			if (CArchMiscWindows::wasLaunchedAsService()) {
				startup = &daemonNTStartup;
			} else {
				startup = &foregroundStartup;
				ARG->m_daemon = false;
			}
		} else {
			startup = &standardStartup;
		}

		// previously we'd send PRINT and FATAL output to a message box, but now
		// that we're using an MS console window for Windows, there's no need really
		//int result = run(__argc, __argv, new CMessageBoxOutputter, startup);
		int result = run(__argc, __argv, NULL, startup);

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
	catch (std::exception& e) {
		showError(instance, __argv[0], IDS_UNCAUGHT_EXCEPTION, e.what());
	}
	catch (...) {
		showError(instance, __argv[0], IDS_UNCAUGHT_EXCEPTION, "<unknown exception>");
	}
	return kExitFailed;
}

#elif SYSAPI_UNIX

int
main(int argc, char** argv)
{
	try {
		int result;
		CArch arch;
		CLOG;
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
	catch (std::exception& e) {
		LOG((CLOG_CRIT "Uncaught exception: %s\n", e.what()));
		throw;
	}
	catch (...) {
		LOG((CLOG_CRIT "Uncaught exception: <unknown exception>\n"));
		throw;
	}
}

#else

#error no main() for platform

#endif

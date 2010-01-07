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
#include "CArchConsoleStd.h"
#include "stdfstream.h"

#include <cstring>
#include <iostream>
#include <sstream>

#if WINAPI_MSWINDOWS
#include "CArchMiscWindows.h"
#include "CMSWindowsScreen.h"
#include "CMSWindowsUtil.h"
#include "CMSWindowsServerTaskBarReceiver.h"
#include "resource.h"
#include "CArchDaemonWindows.h"
#include "CMSWindowsServerApp.h"
#include "CMSWindowsAppUtil.h"
#elif WINAPI_XWINDOWS
#include "CXWindowsScreen.h"
#include "CXWindowsServerTaskBarReceiver.h"
#include "CXWindowsServerApp.h"
#include "CXWindowsAppUtil.h"
#elif WINAPI_CARBON
#include "COSXScreen.h"
#include "COSXServerTaskBarReceiver.h"
#include "COSXServerApp.h"
#include "COSXAppUtil.h"
#endif

// platform dependent name of a daemon
#if SYSAPI_WIN32
#define DAEMON_NAME "Synergy+ Server"
#define DAEMON_INFO "Shares this computers mouse and keyboard with other computers."
#elif SYSAPI_UNIX
#define DAEMON_NAME "synergys"
#endif

#if WINAPI_MSWINDOWS
CMSWindowsServerApp app;
#elif WINAPI_XWINDOWS
CXWindowsServerApp app;
#elif WINAPI_CARBON
COSXServerApp app;
#endif

#define ARG ((CServerApp::CArgs*)&app.args())

//
// platform dependent factories
//

static
CScreen*
createScreen()
{
	app.createScreen();
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

CEvent::Type
getReloadConfigEvent()
{
	return app.getReloadConfigEvent();
}

CEvent::Type
getForceReconnectEvent()
{
	return app.getForceReconnectEvent();
}

CEvent::Type
getResetServerEvent()
{
	return app.getResetServerEvent();
}

void
updateStatus()
{
	app.updateStatus();
}

static
void
updateStatus(const CString& msg)
{
	app.updateStatus(msg);
}

static
void
handleClientConnected(const CEvent& e, void* vlistener)
{
	app.handleClientConnected(e, vlistener);
}

static
CClientListener*
openClientListener(const CNetworkAddress& address)
{
	app.openClientListener(address);
}

static
void
closeClientListener(CClientListener* listen)
{
	app.closeClientListener(listen);
}

static
void
handleScreenError(const CEvent& e, void*)
{
	app.handleScreenError(e, NULL);
}


static void handleSuspend(const CEvent& event, void*);
static void handleResume(const CEvent& event, void*);

static
CScreen*
openServerScreen()
{
	app.openServerScreen();
}

static
void
closeServerScreen(CScreen* screen)
{
	app.closeServerScreen(screen);
}

static
CPrimaryClient*
openPrimaryClient(const CString& name, CScreen* screen)
{
	app.openPrimaryClient(name, screen);
}

static
void
closePrimaryClient(CPrimaryClient* primaryClient)
{
	app.closePrimaryClient(primaryClient);
}

static
void
handleNoClients(const CEvent& e, void*)
{
	app.handleNoClients(e, NULL);
}

static
void
handleClientsDisconnected(const CEvent& e, void*)
{
	app.handleClientsDisconnected(e, NULL);
}

static
CServer*
openServer(const CConfig& config, CPrimaryClient* primaryClient)
{
	app.openServer(config, primaryClient);
}

static
void
closeServer(CServer* server)
{
	app.closeServer(server);
}

static bool initServer();
static bool startServer();

static
void
stopRetryTimer()
{
	app.stopRetryTimer();
}

static
void
retryHandler(const CEvent& e, void*)
{
	app.retryHandler(e, NULL);
}

static
bool
initServer()
{
	app.initServer();
}

static
bool
startServer()
{
	return app.startServer();
}

static
void
stopServer()
{
	app.stopServer();
}

void
cleanupServer()
{
	app.cleanupServer();
}

static
void
handleSuspend(const CEvent& e, void*)
{
	app.handleSuspend(e, NULL);
}

static
void
handleResume(const CEvent& e, void*)
{
	app.handleResume(e, NULL);
}

void
reloadSignalHandler(CArch::ESignal s, void*)
{
	app.reloadSignalHandler(s, NULL);
}

void
reloadConfig(const CEvent& e, void*)
{
	app.reloadConfig(e, NULL);
}

void
forceReconnect(const CEvent& e, void*)
{
	app.forceReconnect(e, NULL);
}

// simply stops and starts the server in order to try and
// work around issues like the sticky meta keys problem, etc
void 
resetServer(const CEvent& e, void*)
{
	app.resetServer(e, NULL);
}

int
mainLoop()
{
	return app.mainLoop();
}

// used by windows nt (service mode)
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

// used by unix and win95
static
int
standardStartup(int argc, char** argv)
{

	// parse command line
	app.parseArgs(argc, argv);

	// load configuration
	app.loadConfig();

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
	// use heap memory because CLog deletes outputters on destruction
	CBufferedLogOutputter* logBuffer = new CBufferedLogOutputter(1000);
	CLOG->insert(logBuffer, true);

	// make the task bar receiver.  the user can control this app
	// through the task bar.
	app.s_taskBarReceiver = createTaskBarReceiver(logBuffer);

	// run
	int result = startup(argc, argv);

	// done with task bar receiver
	delete app.s_taskBarReceiver;

	delete ARG->m_config;
	delete ARG->m_synergyAddress;
	return result;
}


//
// command line parsing
//

bool
loadConfig(const CString& pathname)
{
	return app.loadConfig(pathname);
}

void
loadConfig()
{
	app.loadConfig();
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
	app.parseArgs(argc, argv);
	ARG->m_backend = false;
	loadConfig();
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

// used by windows nt (foreground)
static
int
foregroundStartup(int argc, char** argv)
{
	// parse command line
	app.parseArgs(argc, argv);

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
	LOG((CLOG_ERR "%s", msg.c_str()));
	app.m_bye(kExitFailed);
}

int main(int argc, char** argv) {
	
	app.m_daemonName = DAEMON_NAME;
	app.m_daemonInfo = DAEMON_INFO;
	app.util().m_instance = GetModuleHandle(NULL);

	int code;
	if (app.util().m_instance) {
		code = WinMain(app.util().m_instance, NULL, GetCommandLine(), SW_SHOWNORMAL);
	} else {
		code = kExitFailed;
	}
	
	if (code != kExitSuccess) {
		app.m_bye(code);
	}

	return kExitSuccess;
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	// instantiate before the try-catch block, so they are destroyed after the
	// exception handling (we may need to log exception message). these should
	// not throw exceptions at this stage, so this code should be pretty safe.
	CArch arch(instance);
	CLOG;

	try {
		CArchMiscWindows::setIcons((HICON)LoadImage(instance,
									MAKEINTRESOURCE(IDI_SYNERGY),
									IMAGE_ICON,
									32, 32, LR_SHARED),
									(HICON)LoadImage(instance,
									MAKEINTRESOURCE(IDI_SYNERGY),
									IMAGE_ICON,
									16, 16, LR_SHARED));

		CMSWindowsScreen::init(instance);
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
	CArch arch;
	CLOG;

	try {
		int result;
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

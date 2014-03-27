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

#include "synergy/App.h"
#include "base/Log.h"
#include "common/Version.h"
#include "synergy/protocol_types.h"
#include "arch/Arch.h"
#include "base/XBase.h"
#include "arch/XArch.h"
#include "base/log_outputters.h"
#include "synergy/XSynergy.h"
#include "synergy/ArgsBase.h"
#include "ipc/IpcServerProxy.h"
#include "base/TMethodEventJob.h"
#include "ipc/IpcMessage.h"
#include "ipc/Ipc.h"
#include "base/EventQueue.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#include "base/IEventQueue.h"
#include "base/TMethodJob.h"
#endif

#include <iostream>
#include <stdio.h>

#if WINAPI_CARBON
#include <ApplicationServices/ApplicationServices.h>
#endif

#if defined(__APPLE__)
#include "platform/OSXDragSimulator.h"
#endif

CApp* CApp::s_instance = nullptr;

//
// CMinimalApp
//

CMinimalApp::CMinimalApp(CArgsBase* args) :
	m_bye(&exit),
	m_args(args)
{
}

CMinimalApp::~CMinimalApp()
{
	delete m_args;
}

void
CMinimalApp::version()
{
	printf(
		"%s %s, protocol version %d.%d\n%s\n",
		argsBase().m_pname,
		kVersion,
		kProtocolMajorVersion,
		kProtocolMinorVersion,
		kCopyright);
}

bool
CMinimalApp::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (isArg(i, argc, argv, "-d", "--debug", 1)) {
		// change logging level
		argsBase().m_logFilter = argv[++i];
	}

	else if (isArg(i, argc, argv, "-l", "--log", 1)) {
		argsBase().m_logFile = argv[++i];
	}

	else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
		// not a daemon
		argsBase().m_daemon = false;
	}

	else if (isArg(i, argc, argv, NULL, "--daemon")) {
		// daemonize
		argsBase().m_daemon = true;
	}

	else if (isArg(i, argc, argv, "-h", "--help")) {
		help();
		m_bye(kExitSuccess);
	}

	else if (isArg(i, argc, argv, NULL, "--version")) {
		version();
		m_bye(kExitSuccess);
	}
	
	else {
		// option not supported here
		return false;
	}

	return true;
}

bool
CMinimalApp::isArg(
	int argi, int argc, const char* const* argv,
	const char* name1, const char* name2,
	int minParams)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
			// match.  check args left.
			if (argi + minParams >= argc) {
				LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
					argsBase().m_pname, argv[argi], argsBase().m_pname));
				m_bye(kExitArgs);
			}
			return true;
	}

	// no match
	return false;
}

void
CMinimalApp::parseArgs(int argc, const char* const* argv, int& i)
{
	assert(argv != NULL);
	assert(argc >= 1);

	argsBase().m_pname = ARCH->getBasename(argv[0]);
	argsBase().m_name = ARCH->getHostName();

	// parse options
	for (i = 1; i < argc; ++i) {

		if (parseArg(argc, argv, i)) {
			continue;
		}

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			std::cerr << "Unrecognized option: " << argv[i] << std::endl;
			m_bye(kExitArgs);
		}

		else {
			// this and remaining arguments are not options
			break;
		}
	}
}


//
// CApp
//

CApp::CApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver, CArgsBase* args) :
	m_minimal(args),
	m_taskBarReceiver(NULL),
	m_suspended(false),
	m_events(events),
	m_createTaskBarReceiver(createTaskBarReceiver),
	m_appUtil(events),
	m_ipcClient(nullptr)
{
	assert(s_instance == nullptr);
	s_instance = this;
}

CApp::~CApp()
{
}

CArgsBase&
CApp::argsBase() const
{
	m_minimal.argsBase();
}

void
CApp::setByeFunc(void(*bye)(int))
{
	m_minimal.setByeFunc(bye);
}

void
CApp::bye(int error)
{
	m_minimal.bye(error);
}

bool
CApp::isArg(int argi, int argc, const char* const* argv, const char* name1,
	const char* name2, int minParams)
{
	m_minimal.isArg(argi, argc, argv, name1, name2, minParams);
}

void
CApp::parseArgs(int argc, const char* const* argv)
{
	m_minimal.parseArgs(argc, argv);
}

bool
CApp::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (appUtil().parseArg(argc, argv, i)) {
		// handled by platform util
		return true;
	}

	if (m_minimal.parseArg(argc, argv, i)) {
		// handled by minimal
		return true;
	}
	
	else if (isArg(i, argc, argv, "-n", "--name", 1)) {
		// save screen name
		argsBase().m_name = argv[++i];
	}

	else if (isArg(i, argc, argv, "-1", "--no-restart")) {
		// don't try to restart
		argsBase().m_restartable = false;
	}

	else if (isArg(i, argc, argv, NULL, "--restart")) {
		// try to restart
		argsBase().m_restartable = true;
	}

	else if (isArg(i, argc, argv, "-z", NULL)) {
		argsBase().m_backend = true;
	}

	else if (isArg(i, argc, argv, NULL, "--no-hooks")) {
		argsBase().m_noHooks = true;
	}
	
	else if (isArg(i, argc, argv, NULL, "--no-tray")) {
		argsBase().m_disableTray = true;
	}

	else if (isArg(i, argc, argv, NULL, "--ipc")) {
		argsBase().m_enableIpc = true;
	}

	else if (isArg(i, argc, argv, NULL, "--server")) {
		// HACK: stop error happening when using portable (synergyp) 
	}

	else if (isArg(i, argc, argv, NULL, "--client")) {
		// HACK: stop error happening when using portable (synergyp) 
	}

	else if (isArg(i, argc, argv, NULL, "--crypto-pass")) {
		argsBase().m_crypto.m_pass = argv[++i];
		argsBase().m_crypto.setMode("cfb");
	}

	else if (isArg(i, argc, argv, NULL, "--enable-drag-drop")) {
        bool useDragDrop = true;

#ifdef WINAPI_XWINDOWS

        useDragDrop = false;
		LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported on linux."));

#endif

#ifdef WINAPI_MSWINDOWS

        OSVERSIONINFO osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);

        if (osvi.dwMajorVersion < 6) {
            useDragDrop = false;
		    LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported below vista."));
        }
#endif

        if (useDragDrop) {
		    argsBase().m_enableDragDrop = true;
        }
	}

	else {
		// option not supported here
		return false;
	}

	return true;
}

void
CApp::parseArgs(int argc, const char* const* argv, int& i)
{
	m_minimal.parseArgs(argc, argv, i);

#if SYSAPI_WIN32
	// suggest that user installs as a windows service. when launched as 
	// service, process should automatically detect that it should run in
	// daemon mode.
	if (argsBase().m_daemon) {
		LOG((CLOG_ERR 
			"the --daemon argument is not supported on windows. "
			"instead, install %s as a service (--service install)", 
			argsBase().m_pname));
		bye(kExitArgs);
	}
#endif
}

int
CApp::run(int argc, char** argv)
{	
#if MAC_OS_X_VERSION_10_7
	// dock hide only supported on lion :(
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	GetCurrentProcess(&psn);
#pragma GCC diagnostic pop

	TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
#endif

	// install application in to arch
	appUtil().adoptApp(this);
	
	// HACK: fail by default (saves us setting result in each catch)
	int result = kExitFailed;

	try {
		result = appUtil().run(argc, argv);
	}
	catch (XExitApp& e) {
		// instead of showing a nasty error, just exit with the error code.
		// not sure if i like this behaviour, but it's probably better than 
		// using the exit(int) function!
		result = e.getCode();
	}
	catch (std::exception& e) {
		LOG((CLOG_CRIT "An error occurred: %s\n", e.what()));
	}
	catch (...) {
		LOG((CLOG_CRIT "An unknown error occurred.\n"));
	}

	appUtil().beforeAppExit();
	
	return result;
}

int
CApp::daemonMainLoop(int, const char**)
{
#if SYSAPI_WIN32
	CSystemLogger sysLogger(daemonName(), false);
#else
	CSystemLogger sysLogger(daemonName(), true);
#endif
	return mainLoop();
}

void 
CApp::setupFileLogging()
{
	if (argsBase().m_logFile != NULL) {
		m_fileLog = new CFileLogOutputter(argsBase().m_logFile);
		CLOG->insert(m_fileLog);
		LOG((CLOG_DEBUG1 "logging to file (%s) enabled", argsBase().m_logFile));
	}
}

void 
CApp::loggingFilterWarning()
{
	if (CLOG->getFilter() > CLOG->getConsoleMaxLevel()) {
		if (argsBase().m_logFile == NULL) {
			LOG((CLOG_WARN "log messages above %s are NOT sent to console (use file logging)", 
				CLOG->getFilterName(CLOG->getConsoleMaxLevel())));
		}
	}
}

void 
CApp::initApp(int argc, const char** argv)
{
	// parse command line
	parseArgs(argc, argv);

	// setup file logging after parsing args
	setupFileLogging();

	// load configuration
	loadConfig();

	if (!argsBase().m_disableTray) {

		// create a log buffer so we can show the latest message
		// as a tray icon tooltip
		CBufferedLogOutputter* logBuffer = new CBufferedLogOutputter(1000);
		CLOG->insert(logBuffer, true);

		// make the task bar receiver.  the user can control this app
		// through the task bar.
		m_taskBarReceiver = m_createTaskBarReceiver(logBuffer, m_events);
	}
}

void
CApp::initIpcClient()
{
	m_ipcClient = new CIpcClient(m_events, m_socketMultiplexer);
	m_ipcClient->connect();

	m_events->adoptHandler(
		m_events->forCIpcClient().messageReceived(), m_ipcClient,
		new TMethodEventJob<CApp>(this, &CApp::handleIpcMessage));
}

void
CApp::cleanupIpcClient()
{
	m_ipcClient->disconnect();
	m_events->removeHandler(m_events->forCIpcClient().messageReceived(), m_ipcClient);
	delete m_ipcClient;
}

void
CApp::handleIpcMessage(const CEvent& e, void*)
{
	CIpcMessage* m = static_cast<CIpcMessage*>(e.getDataObject());
	if (m->type() == kIpcShutdown) {
		LOG((CLOG_INFO "got ipc shutdown message"));
		m_events->addEvent(CEvent(CEvent::kQuit));
    }
}

void
CApp::runEventsLoop(void*)
{
	m_events->loop();
	
#if defined(MAC_OS_X_VERSION_10_7)
	
	stopCocoaLoop();
	
#endif
}

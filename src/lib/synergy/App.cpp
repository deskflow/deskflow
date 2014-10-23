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
// CApp
//

CApp::CApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver, CArgsBase* args) :
	m_bye(&exit),
	m_taskBarReceiver(NULL),
	m_suspended(false),
	m_events(events),
	m_args(args),
	m_createTaskBarReceiver(createTaskBarReceiver),
	m_appUtil(events),
	m_ipcClient(nullptr)
{
	assert(s_instance == nullptr);
	s_instance = this;
}

CApp::~CApp()
{
	s_instance = nullptr;
	delete m_args;
}

void
CApp::version()
{
	char buffer[500];
	sprintf(
		buffer,
		"%s %s, protocol version %d.%d\n%s",
		argsBase().m_pname,
		kVersion,
		kProtocolMajorVersion,
		kProtocolMinorVersion,
		kCopyright
		);

	std::cout << buffer << std::endl;
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

	// set log filter
	if (!CLOG->setFilter(argsBase().m_logFilter)) {
		LOG((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
			argsBase().m_pname, argsBase().m_logFilter, argsBase().m_pname));
		m_bye(kExitArgs);
	}
	loggingFilterWarning();
	
	if (argsBase().m_enableDragDrop) {
		LOG((CLOG_INFO "drag and drop enabled"));
	}

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

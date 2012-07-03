/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Nick Bolton
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

// TODO: split this class into windows and unix to get rid
// of all the #ifdefs!

#include "CDaemonApp.h"
#include "CEventQueue.h"
#include "LogOutputters.h"
#include "CLog.h"
#include "XArch.h"
#include "CApp.h"

#include <string>
#include <iostream>
#include <sstream>

#if SYSAPI_WIN32

#include "CArchMiscWindows.h"
#include "XArchWindows.h"
#include "CScreen.h"
#include "CMSWindowsScreen.h"
#include "CMSWindowsRelauncher.h"
#include "CMSWindowsDebugOutputter.h"
#include "TMethodJob.h"
#include "TMethodEventJob.h"
#include "CIpcClientProxy.h"
#include "CIpcMessage.h"
#include "CSocketMultiplexer.h"
#include "CIpcLogOutputter.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

using namespace std;

CDaemonApp* CDaemonApp::s_instance = NULL;

int
mainLoopStatic()
{
	CDaemonApp::s_instance->mainLoop(true);
	return kExitSuccess;
}

int
unixMainLoopStatic(int, const char**)
{
	return mainLoopStatic();
}

#if SYSAPI_WIN32
int
winMainLoopStatic(int, const char**)
{
	return CArchMiscWindows::runDaemon(mainLoopStatic);
}
#endif

CDaemonApp::CDaemonApp() :
m_ipcServer(nullptr),
m_ipcLogOutputter(nullptr)
#if SYSAPI_WIN32
,m_relauncher(false)
#endif
{
	s_instance = this;
}

CDaemonApp::~CDaemonApp()
{
}

int
CDaemonApp::run(int argc, char** argv)
{
	try
	{
#if SYSAPI_WIN32
		// win32 instance needed for threading, etc.
		CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

#if SYSAPI_WIN32
		// sends debug messages to visual studio console window.
		CLOG->insert(new CMSWindowsDebugOutputter());
#endif

		// default log level to system setting.
		string logLevel = ARCH->setting("LogLevel");
		if (logLevel != "")
			CLOG->setFilter(logLevel.c_str());

		bool foreground = false;

		for (int i = 1; i < argc; ++i) {
			string arg(argv[i]);

			if (arg == "/f" || arg == "-f") {
				foreground = true;
			}
#if SYSAPI_WIN32
			else if (arg == "/install") {
				ARCH->installDaemon();
				return kExitSuccess;
			}
			else if (arg == "/uninstall") {
				ARCH->uninstallDaemon();
				return kExitSuccess;
			}
#endif
			else {
				stringstream ss;
				ss << "Unrecognized argument: " << arg;
				foregroundError(ss.str().c_str());
				return kExitArgs;
			}
		}

		if (foreground) {
			// run process in foreground instead of daemonizing.
			// useful for debugging.
			mainLoop(false);
		}
		else {
#if SYSAPI_WIN32
			ARCH->daemonize("Synergy", winMainLoopStatic);
#elif SYSAPI_UNIX
			ARCH->daemonize("Synergy", unixMainLoopStatic);
#endif
		}

		return kExitSuccess;
	}
	catch (XArch& e) {
		foregroundError(e.what().c_str());
		return kExitFailed;
	}
	catch (std::exception& e) {
		foregroundError(e.what());
		return kExitFailed;
	}
	catch (...) {
		foregroundError("Unrecognized error.");
		return kExitFailed;
	}
}

void
CDaemonApp::mainLoop(bool logToFile)
{
	try
	{
		DAEMON_RUNNING(true);

		if (logToFile)
			CLOG->insert(new CFileLogOutputter(logPath().c_str()));

		CEventQueue eventQueue;

		// create socket multiplexer.  this must happen after daemonization
		// on unix because threads evaporate across a fork().
		CSocketMultiplexer multiplexer;

		// uses event queue, must be created here.
		m_ipcServer = new CIpcServer();

		eventQueue.adoptHandler(
			CIpcServer::getClientConnectedEvent(), m_ipcServer,
			new TMethodEventJob<CDaemonApp>(this, &CDaemonApp::handleIpcConnected));

		m_ipcServer->listen();

		// send logging to gui via ipc, log system adopts outputter.
		m_ipcLogOutputter = new CIpcLogOutputter(*m_ipcServer);
		CLOG->insert(m_ipcLogOutputter);

#if SYSAPI_WIN32
		// HACK: create a dummy screen, which can handle system events 
		// (such as a stop request from the service controller).
		CMSWindowsScreen::init(CArchMiscWindows::instanceWin32());
		CGameDeviceInfo gameDevice;
		CScreen dummyScreen(new CMSWindowsScreen(false, true, gameDevice));

		m_relauncher.m_ipcLogOutputter = m_ipcLogOutputter;

		string command = ARCH->setting("Command");
		if (command != "") {
			LOG((CLOG_INFO "using last known command: %s", command.c_str()));
			m_relauncher.command(command);
		}

		m_relauncher.startAsync();
#endif

		eventQueue.loop();

#if SYSAPI_WIN32
		m_relauncher.stop();
#endif

		eventQueue.removeHandler(
			CIpcServer::getClientConnectedEvent(), m_ipcServer);
		
		delete m_ipcServer;

		DAEMON_RUNNING(false);
	}
	catch (XArch& e) {
		LOG((CLOG_ERR "xarch exception: %s", e.what().c_str()));
	}
	catch (std::exception& e) {
		LOG((CLOG_ERR "std exception: %s", e.what()));
	}
	catch (...) {
		LOG((CLOG_ERR "unrecognized error."));
	}
}

void
CDaemonApp::foregroundError(const char* message)
{
#if SYSAPI_WIN32
	MessageBox(NULL, message, "Synergy Service", MB_OK | MB_ICONERROR);
#elif SYSAPI_UNIX
	cerr << message << endl;
#endif
}

std::string
CDaemonApp::logPath()
{
#ifdef SYSAPI_WIN32
	// TODO: move to CArchMiscWindows
	// on windows, log to the same dir as the binary.
	char fileNameBuffer[MAX_PATH];
	GetModuleFileName(NULL, fileNameBuffer, MAX_PATH);
	string fileName(fileNameBuffer);
	size_t lastSlash = fileName.find_last_of("\\");
	string path(fileName.substr(0, lastSlash));
	path.append("\\").append(LOG_FILENAME);
	return path;
#elif SYSAPI_UNIX
	return "/var/log/" LOG_FILENAME;
#endif
}

void
CDaemonApp::handleIpcConnected(const CEvent& e, void*)
{
	LOG((CLOG_INFO "ipc client connected"));
	EVENTQUEUE->adoptHandler(
		CIpcClientProxy::getMessageReceivedEvent(), e.getData(),
		new TMethodEventJob<CDaemonApp>(
		this, &CDaemonApp::handleIpcMessage));
}

void
CDaemonApp::handleIpcMessage(const CEvent& e, void*)
{
	CIpcMessage& m = *reinterpret_cast<CIpcMessage*>(e.getData());

	LOG((CLOG_DEBUG "ipc message, type=%d", m.m_type));

	switch (m.m_type) {
		case kIpcCommand: {
			CString& command = *reinterpret_cast<CString*>(m.m_data);
			LOG((CLOG_DEBUG "got new command: %s", command.c_str()));

			try {
				// store command in system settings. this is used when the daemon
				// next starts.
				ARCH->setting("Command", command);
			}
			catch (XArch& e) {
				LOG((CLOG_ERR "failed to save setting, %s", e.what().c_str()));
			}
				
			// tell the relauncher about the new command. this causes the
			// relauncher to stop the existing command and start the new
			// command.
			m_relauncher.command(command);
		}
		break;

		default:
			LOG((CLOG_ERR "ipc message not supported, type=%d", m.m_type));
			break;
	}
}

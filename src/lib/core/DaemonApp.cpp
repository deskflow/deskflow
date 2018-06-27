/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// TODO(andrew): split this class into windows and unix to get rid
// of all the #ifdefs!

#include "core/DaemonApp.h"

#include "arch/XArch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "base/TMethodJob.h"
#include "base/log_outputters.h"
#include "core/App.h"
#include "core/ArgParser.h"
#include "core/ClientArgs.h"
#include "core/ServerArgs.h"
#include "net/SocketMultiplexer.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"
#include "core/Screen.h"
#include "platform/MSWindowsScreen.h"
#include "platform/MSWindowsDebugOutputter.h"
#include "platform/MSWindowsEventQueueBuffer.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

DaemonApp* DaemonApp::s_instance = nullptr;

int
mainLoopStatic()
{
    DaemonApp::s_instance->mainLoop(true);
    return kExitSuccess;
}

int
unixMainLoopStatic(int /*unused*/, const char** /*unused*/)
{
    return mainLoopStatic();
}

#if SYSAPI_WIN32
int
winMainLoopStatic(int, const char**)
{
    return ArchMiscWindows::runDaemon(mainLoopStatic);
}
#endif

DaemonApp::DaemonApp() :
    #if SYSAPI_WIN32
    m_watchdog(nullptr),
    #endif
    m_events(nullptr),
    m_fileLogOutputter(nullptr)
{
    s_instance = this;
}

DaemonApp::~DaemonApp()
= default;

int
DaemonApp::run(int argc, char** argv)
{
#if SYSAPI_WIN32
    // win32 instance needed for threading, etc.
    ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif
    
    Arch arch;
    arch.init();

    Log log;
    EventQueue events;
    m_events = &events;

    bool uninstall = false;
    try
    {
#if SYSAPI_WIN32
        // sends debug messages to visual studio console window.
        log.insert(new MSWindowsDebugOutputter());
#endif

        // default log level to system setting.
        string logLevel = arch.setting("LogLevel");
        if (!logLevel.empty()) {
            log.setFilter(logLevel.c_str());
}

        bool foreground = false;

        for (int i = 1; i < argc; ++i) {
            string arg(argv[i]);

            if (arg == "/f" || arg == "-f") {
                foreground = true;
            }
#if SYSAPI_WIN32
            else if (arg == "/install") {
                uninstall = true;
                arch.installDaemon();
                return kExitSuccess;
            }
            else if (arg == "/uninstall") {
                arch.uninstallDaemon();
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
            arch.daemonize("Synergy", winMainLoopStatic);
#elif SYSAPI_UNIX
            arch.daemonize("Synergy", unixMainLoopStatic);
#endif
        }

        return kExitSuccess;
    }
    catch (XArch& e) {
        String message = e.what();
        if (uninstall && (message.find("The service has not been started") != String::npos)) {
            // TODO(andrew): if we're keeping this use error code instead (what is it?!).
            // HACK: this message happens intermittently, not sure where from but
            // it's quite misleading for the user. they thing something has gone
            // horribly wrong, but it's just the service manager reporting a false
            // positive (the service has actually shut down in most cases).
        }
        else {
            foregroundError(message.c_str());
        }
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
DaemonApp::mainLoop(bool logToFile)
{
    try
    {
        DAEMON_RUNNING(true);
        
        if (logToFile) {
            m_fileLogOutputter = new FileLogOutputter(logFilename().c_str());
            CLOG->insert(m_fileLogOutputter);
        }

        // create socket multiplexer.  this must happen after daemonization
        // on unix because threads evaporate across a fork().
        SocketMultiplexer multiplexer;
        m_events->loop();

        DAEMON_RUNNING(false);
    }
    catch (std::exception& e) {
        LOG((CLOG_CRIT "An error occurred: %s", e.what()));
    }
    catch (...) {
        LOG((CLOG_CRIT "An unknown error occurred.\n"));
    }
}

void
DaemonApp::foregroundError(const char* message)
{
#if SYSAPI_WIN32
    MessageBox(NULL, message, "Synergy Service", MB_OK | MB_ICONERROR);
#elif SYSAPI_UNIX
    cerr << message << endl;
#endif
}

std::string
DaemonApp::logFilename()
{
    string logFilename;
    logFilename = ARCH->setting("LogFilename");
    if (logFilename.empty()) {
        logFilename = ARCH->getLogDirectory();
        logFilename.append("/");
        logFilename.append(LOG_FILENAME);
    }

    return logFilename;
}

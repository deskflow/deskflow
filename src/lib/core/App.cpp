/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "core/App.h"

#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "base/XBase.h"
#include "base/log_outputters.h"
#include "common/Version.h"
#include "core/ArgsBase.h"
#include "core/XSynergy.h"
#include "core/protocol_types.h"

#include <iostream>
#include <cstdio>
#include "DisplayInvalidException.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#include "base/IEventQueue.h"
#include "base/TMethodJob.h"
#endif

#if WINAPI_CARBON
#include <ApplicationServices/ApplicationServices.h>
#include "platform/OSXDragSimulator.h"
#endif

#if WINAPI_XWINDOWS
#include <unistd.h>
#endif


App* App::s_instance = nullptr;

//
// App
//

App::App(IEventQueue* events, ArgsBase* args) :
    m_bye(&exit),
    m_suspended(false),
    m_events(events),
    m_args(args),
    m_fileLog(nullptr),
    m_appUtil(events),
    m_socketMultiplexer(nullptr)
{
    assert(s_instance == nullptr);
    s_instance = this;
}

App::~App()
{
    s_instance = nullptr;
    delete m_args;
}

void
App::version()
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
App::run(int argc, char** argv)
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
    catch (DisplayInvalidException& die) {
        LOG((CLOG_CRIT "A display invalid exception error occurred: %s\n", die.what()));
        // display invalid exceptions can occur when going to sleep. When this process exits, the
        // UI will restart us instantly. We don't really want that behevior, so we quies for a bit
        (void)sleep(10);
    }
    catch (std::runtime_error& re) {
        LOG((CLOG_CRIT "A runtime error occurred: %s\n", re.what()));
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
App::daemonMainLoop(int /*unused*/, const char** /*unused*/)
{
#if SYSAPI_WIN32
    SystemLogger sysLogger(daemonName(), false);
#else
    SystemLogger sysLogger(daemonName(), true);
#endif
    return mainLoop();
}

void 
App::setupFileLogging()
{
    if (argsBase().m_logFile != nullptr) {
        m_fileLog = new FileLogOutputter(argsBase().m_logFile);
        CLOG->insert(m_fileLog);
        LOG((CLOG_DEBUG1 "logging to file (%s) enabled", argsBase().m_logFile));
    }
}

void 
App::loggingFilterWarning()
{
    if (CLOG->getFilter() > CLOG->getConsoleMaxLevel()) {
        if (argsBase().m_logFile == nullptr) {
            LOG((CLOG_WARN "log messages above %s are NOT sent to console (use file logging)", 
                CLOG->getFilterName(CLOG->getConsoleMaxLevel())));
        }
    }
}

void 
App::initApp(int argc, const char** argv)
{
    // parse command line
    parseArgs(argc, argv);

#if WINAPI_XWINDOWS
    // for use on linux, tell the core process what user id it should run as.
    // this is a simple way to allow the core process to talk to X. this avoids
    // the "WARNING: primary screen unavailable: unable to open screen" error.
    // a better way would be to use xauth cookie and dbus to get access to X.
    if (argsBase().m_runAsUid >= 0) {
        if (setuid(argsBase().m_runAsUid) == 0) {
            LOG((CLOG_DEBUG "process uid was set to: %d", argsBase().m_runAsUid));
        }
        else {
            LOG((CLOG_WARN "failed to set process uid to: %d", argsBase().m_runAsUid));
        }
    }
#endif
    
    ARCH->setProfileDirectory(argsBase().m_profileDirectory);
    ARCH->setPluginDirectory(argsBase().m_pluginDirectory);

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
}

void
App::runEventsLoop(void* /*unused*/)
{
    m_events->loop();
    
#if defined(MAC_OS_X_VERSION_10_7)
    
    stopCocoaLoop();
    
#endif
}

//
// MinimalApp
//

MinimalApp::MinimalApp() :
    App(nullptr, new ArgsBase())
{
    m_arch.init();
    setEvents(m_events);
}

MinimalApp::~MinimalApp()
= default;

int
MinimalApp::standardStartup(int  /*argc*/, char**  /*argv*/)
{
    return 0;
}

int
MinimalApp::runInner(int  /*argc*/, char**  /*argv*/, ILogOutputter*  /*outputter*/, StartupFunc  /*startup*/)
{
    return 0;
}

void
MinimalApp::startNode()
{
}

int
MinimalApp::mainLoop()
{
    return 0;
}

int
MinimalApp::foregroundStartup(int  /*argc*/, char**  /*argv*/)
{
    return 0;
}

synergy::Screen*
MinimalApp::createScreen()
{
    return nullptr;
}

void
MinimalApp::loadConfig()
{
}

bool
MinimalApp::loadConfig(const String&  /*pathname*/)
{
    return false;
}

const char*
MinimalApp::daemonInfo() const
{
    return "";
}

const char*
MinimalApp::daemonName() const
{
    return "";
}

void
MinimalApp::parseArgs(int  /*argc*/, const char* const*  /*argv*/)
{
}

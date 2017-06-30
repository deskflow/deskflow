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

#pragma once

#include "arch/Arch.h"
#include "ipc/IpcServer.h"

#include <string>

class Event;
class IpcLogOutputter;
class FileLogOutputter;

#if SYSAPI_WIN32
class MSWindowsWatchdog;
#endif

class DaemonApp {

public:
    DaemonApp ();
    virtual ~DaemonApp ();
    int run (int argc, char** argv);
    void mainLoop (bool logToFile);

private:
    void daemonize ();
    void foregroundError (const char* message);
    std::string logFilename ();
    void handleIpcMessage (const Event&, void*);

public:
    static DaemonApp* s_instance;

#if SYSAPI_WIN32
    MSWindowsWatchdog* m_watchdog;
#endif

private:
    IpcServer* m_ipcServer;
    IpcLogOutputter* m_ipcLogOutputter;
    IEventQueue* m_events;
    FileLogOutputter* m_fileLogOutputter;
};

#define LOG_FILENAME "synergyd.log"

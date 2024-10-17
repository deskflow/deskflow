/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#include "ipc/IpcServer.h"

#include <memory>
#include <string>

class Event;
class IpcLogOutputter;
class FileLogOutputter;

#if SYSAPI_WIN32
class MSWindowsWatchdog;
#endif

extern const char *const kLogFilename;

class DaemonApp
{

public:
  DaemonApp();
  ~DaemonApp();
  int run(int argc, char **argv);
  void mainLoop(bool logToFile, bool foreground = false);

private:
  void daemonize();
  void foregroundError(const char *message);
  std::string logFilename();
  void handleIpcMessage(const Event &, void *);

public:
  static DaemonApp *s_instance;

#if SYSAPI_WIN32
  std::unique_ptr<MSWindowsWatchdog> m_watchdog;
#endif

private:
  std::unique_ptr<IpcServer> m_ipcServer;
  std::unique_ptr<IpcLogOutputter> m_ipcLogOutputter;
  std::unique_ptr<IEventQueue> m_events;
  std::unique_ptr<FileLogOutputter> m_fileLogOutputter;
};

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

// TODO: split this class into windows and unix to get rid
// of all the #ifdefs!

#include "deskflow/DaemonApp.h"

#include "arch/XArch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "base/log_outputters.h"
#include "common/ipc.h"
#include "deskflow/App.h"
#include "deskflow/ArgParser.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/ServerArgs.h"
#include "ipc/IpcClientProxy.h"
#include "ipc/IpcLogOutputter.h"
#include "ipc/IpcMessage.h"
#include "ipc/IpcSettingMessage.h"
#include "net/SocketMultiplexer.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchMiscWindows.h"
#include "deskflow/Screen.h"
#include "platform/MSWindowsDebugOutputter.h"
#include "platform/MSWindowsEventQueueBuffer.h"
#include "platform/MSWindowsWatchdog.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#elif SYSAPI_UNIX

#include <iostream>

#endif

#include <memory>
#include <sstream>
#include <string>

using namespace std;

const char *const kLogFilename = DAEMON_BINARY_NAME ".log";

namespace {
void updateSetting(const IpcMessage &message)
{
  try {
    auto setting = static_cast<const IpcSettingMessage &>(message);
    ARCH->setting(setting.getName(), setting.getValue());
  } catch (const XArch &e) {
    LOG((CLOG_ERR "failed to save setting: %s", e.what()));
  }
}

bool isServerCommandLine(const std::vector<String> &cmd)
{
  auto isServer = false;

  if (cmd.size() > 1) {
    isServer = (cmd[0].find("deskflow-server") != String::npos) ||
               (cmd[0].find(CORE_BINARY_NAME) != String::npos && cmd[1] == "server");
  }

  return isServer;
}

} // namespace

DaemonApp *DaemonApp::s_instance = nullptr;

int mainLoopStatic()
{
  DaemonApp::s_instance->mainLoop(true);
  return kExitSuccess;
}

int unixMainLoopStatic(int, const char **)
{
  return mainLoopStatic();
}

#if SYSAPI_WIN32
int winMainLoopStatic(int, const char **)
{
  return ArchMiscWindows::runDaemon(mainLoopStatic);
}
#endif

DaemonApp::DaemonApp()
{
  s_instance = this;
}

DaemonApp::~DaemonApp()
{
  s_instance = nullptr;
}

int DaemonApp::run(int argc, char **argv)
{
#if SYSAPI_WIN32
  // win32 instance needed for threading, etc.
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

  Arch arch;
  arch.init();

  Log log;
  m_events = std::make_unique<EventQueue>();

  bool uninstall = false;
  try {
#if SYSAPI_WIN32
    // TODO: maybe we should only add this if not using /f?
    // sends debug messages to visual studio console window.
    log.insert(new MSWindowsDebugOutputter());
#endif

    // default log level to system setting.
    if (string logLevel = arch.setting("LogLevel"); logLevel != "")
      log.setFilter(logLevel.c_str());

    bool foreground = false;

    for (int i = 1; i < argc; ++i) {
      string arg(argv[i]);

      if (arg == "/f" || arg == "-f") {
        foreground = true;
      }
#if SYSAPI_WIN32
      else if (arg == "/install") {
        LOG((CLOG_PRINT "installing windows daemon"));
        uninstall = true;
        arch.installDaemon();
        return kExitSuccess;
      } else if (arg == "/uninstall") {
        LOG((CLOG_PRINT "uninstalling windows daemon"));
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
      LOG((CLOG_PRINT "starting daemon in foreground"));

      // run process in foreground instead of daemonizing.
      // useful for debugging.
      mainLoop(false, foreground);
    } else {
#if SYSAPI_WIN32
      LOG((CLOG_PRINT "daemonizing windows service"));
      arch.daemonize("Deskflow", winMainLoopStatic);
#elif SYSAPI_UNIX
      LOG((CLOG_PRINT "daemonizing unix service"));
      arch.daemonize("Deskflow", unixMainLoopStatic);
#endif
    }

    return kExitSuccess;
  } catch (XArch &e) {
    String message = e.what();
    if (uninstall && (message.find("The service has not been started") != String::npos)) {
      // TODO: if we're keeping this use error code instead (what is it?!).
      // HACK: this message happens intermittently, not sure where from but
      // it's quite misleading for the user. they thing something has gone
      // horribly wrong, but it's just the service manager reporting a false
      // positive (the service has actually shut down in most cases).
    } else {
      foregroundError(message.c_str());
    }
    return kExitFailed;
  } catch (std::exception &e) {
    foregroundError(e.what());
    return kExitFailed;
  } catch (...) {
    foregroundError("Unrecognized error.");
    return kExitFailed;
  }
}

void DaemonApp::mainLoop(bool logToFile, bool foreground)
{
  try {
    DAEMON_RUNNING(true);

    if (logToFile) {
      m_fileLogOutputter = std::make_unique<FileLogOutputter>(logFilename().c_str());
      CLOG->insert(m_fileLogOutputter.get());
    }

    // create socket multiplexer.  this must happen after daemonization
    // on unix because threads evaporate across a fork().
    SocketMultiplexer multiplexer;

    // uses event queue, must be created here.
    m_ipcServer = std::make_unique<IpcServer>(m_events.get(), &multiplexer);

    // send logging to gui via ipc, log system adopts outputter.
    m_ipcLogOutputter = std::make_unique<IpcLogOutputter>(*m_ipcServer, IpcClientType::GUI, true);
    CLOG->insert(m_ipcLogOutputter.get());

#if SYSAPI_WIN32
    m_watchdog = std::make_unique<MSWindowsWatchdog>(false, *m_ipcServer, *m_ipcLogOutputter, foreground);
    m_watchdog->setFileLogOutputter(m_fileLogOutputter.get());
#endif

    m_events->adoptHandler(
        m_events->forIpcServer().messageReceived(), m_ipcServer.get(),
        new TMethodEventJob<DaemonApp>(this, &DaemonApp::handleIpcMessage)
    );

    m_ipcServer->listen();

#if SYSAPI_WIN32

    // install the platform event queue to handle service stop events.
    m_events->adoptBuffer(new MSWindowsEventQueueBuffer(m_events.get()));

    String command = ARCH->setting("Command");
    bool elevate = ARCH->setting("Elevate") == "1";
    if (command != "") {
      LOG((CLOG_INFO "using last known command: %s", command.c_str()));
      m_watchdog->setCommand(command, elevate);
    }

    m_watchdog->startAsync();
#endif
    m_events->loop();

#if SYSAPI_WIN32
    m_watchdog->stop();
#endif

    m_events->removeHandler(m_events->forIpcServer().messageReceived(), m_ipcServer.get());

    CLOG->remove(m_ipcLogOutputter.get());

    DAEMON_RUNNING(false);
  } catch (std::exception &e) {
    LOG((CLOG_CRIT "an error occurred: %s", e.what()));
  } catch (...) {
    LOG((CLOG_CRIT "an unknown error occurred.\n"));
  }
}

void DaemonApp::foregroundError(const char *message)
{
#if SYSAPI_WIN32
  MessageBox(NULL, message, "Deskflow Service", MB_OK | MB_ICONERROR);
#elif SYSAPI_UNIX
  cerr << message << endl;
#endif
}

std::string DaemonApp::logFilename()
{
  string logFilename;
  logFilename = ARCH->setting("LogFilename");
  if (logFilename.empty()) {
    logFilename = ARCH->getLogDirectory();
    logFilename.append("/");
    logFilename.append(kLogFilename);
  }

  return logFilename;
}

void DaemonApp::handleIpcMessage(const Event &e, void *)
{
  IpcMessage *m = static_cast<IpcMessage *>(e.getDataObject());
  switch (m->type()) {
  case IpcMessageType::Command: {
    IpcCommandMessage *cm = static_cast<IpcCommandMessage *>(m);
    String command = cm->command();

    // if empty quotes, clear.
    if (command == "\"\"") {
      command.clear();
    }

    if (!command.empty()) {
      LOG((CLOG_DEBUG "daemon got new core command"));
      LOG((CLOG_DEBUG2 "new command, elevate=%d command=%s", cm->elevate(), command.c_str()));

      std::vector<String> argsArray;
      ArgParser::splitCommandString(command, argsArray);
      ArgParser argParser(NULL);
      const char **argv = argParser.getArgv(argsArray);
      int argc = static_cast<int>(argsArray.size());

      if (isServerCommandLine(argsArray)) {
        auto serverArgs = new deskflow::ServerArgs();
        argParser.parseServerArgs(*serverArgs, argc, argv);
      } else {
        auto clientArgs = new deskflow::ClientArgs();
        argParser.parseClientArgs(*clientArgs, argc, argv);
      }

      delete[] argv;
      String logLevel(ArgParser::argsBase().m_logFilter);
      if (!logLevel.empty()) {
        try {
          // change log level based on that in the command string
          // and change to that log level now.
          ARCH->setting("LogLevel", logLevel);
          CLOG->setFilter(logLevel.c_str());
        } catch (XArch &e) {
          LOG((CLOG_ERR "failed to save LogLevel setting, %s", e.what()));
        }
      }
    } else {
      LOG((CLOG_DEBUG "empty command, elevate=%d", cm->elevate()));
    }

    try {
      // store command in system settings. this is used when the daemon
      // next starts.
      ARCH->setting("Command", command);

      // TODO: it would be nice to store bools/ints...
      ARCH->setting("Elevate", String(cm->elevate() ? "1" : "0"));
    } catch (XArch &e) {
      LOG((CLOG_ERR "failed to save settings, %s", e.what()));
    }

#if SYSAPI_WIN32
    // tell the relauncher about the new command. this causes the
    // relauncher to stop the existing command and start the new
    // command.
    m_watchdog->setCommand(command, cm->elevate());
#endif
    break;
  }

  case IpcMessageType::Hello: {
    IpcHelloMessage *hm = static_cast<IpcHelloMessage *>(m);
    String type;
    switch (hm->clientType()) {
    case IpcClientType::GUI:
      type = "gui";
      break;
    case IpcClientType::Node:
      type = "node";
      break;
    default:
      type = "unknown";
      break;
    }

    LOG((CLOG_DEBUG "ipc hello, type=%s", type.c_str()));

    // TODO: implement hello back handling in s1 gui and node (server/client).
    if (hm->clientType() == IpcClientType::GUI) {
      LOG((CLOG_DEBUG "sending ipc hello back"));
      IpcHelloBackMessage hbm;
      m_ipcServer->send(hbm, hm->clientType());
    }

#if SYSAPI_WIN32
    String watchdogStatus = m_watchdog->isProcessActive() ? "active" : "idle";
    LOG((CLOG_INFO "service status: %s", watchdogStatus.c_str()));
#endif

    m_ipcLogOutputter->notifyBuffer();
    break;
  }

  case IpcMessageType::Setting:
    updateSetting(*m);
    break;

  default:
    LOG((CLOG_DEBUG "ipc message ignored"));
    break;
  }
}

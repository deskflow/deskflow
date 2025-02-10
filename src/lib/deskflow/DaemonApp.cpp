/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: split this class into windows and unix to get rid
// of all the #ifdefs!

#include "deskflow/DaemonApp.h"

#include "arch/Arch.h"
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

#include "arch/win32/ArchMiscWindows.h" // IWYU pragma: keep
#include "deskflow/Screen.h"
#include "platform/MSWindowsDebugOutputter.h"
#include "platform/MSWindowsEventQueueBuffer.h"
#include "platform/MSWindowsWatchdog.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace std;
using namespace deskflow::core;

const char *const kLogFilename = "deskflow-daemon.log";

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

bool isServerCommandLine(const std::vector<std::string> &cmd)
{
  auto isServer = false;

  if (cmd.size() > 1) {
    isServer = (cmd[0].find("deskflow-server") != std::string::npos) ||
               (cmd[0].find("deskflow-core") != std::string::npos && cmd[1] == "server");
  }

  return isServer;
}

} // namespace

DaemonApp *DaemonApp::s_instance = nullptr;

int mainLoopStatic()
{
  DaemonApp::instance()->mainLoop(true);
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

void DaemonApp::run()
{
  if (m_foreground) {
    LOG((CLOG_NOTE "starting daemon in foreground"));

    // run process in foreground instead of daemonizing.
    // useful for debugging.
    mainLoop(false, m_foreground);
  } else {
#if SYSAPI_WIN32
    LOG((CLOG_NOTE "daemonizing windows service"));
    ARCH->daemonize(kAppName, winMainLoopStatic);
#elif SYSAPI_UNIX
    LOG((CLOG_NOTE "daemonizing unix service"));
    ARCH->daemonize(kAppName, unixMainLoopStatic);
#endif
  }
}

void DaemonApp::setLogLevel(const QString &logLevel)
{
  LOG((CLOG_DEBUG "log level changed: %s", logLevel.toUtf8().constData()));
  CLOG->setFilter(logLevel.toUtf8().constData());

  try {
    // saves setting for next time the daemon starts.
    ARCH->setting("LogLevel", logLevel.toStdString());
  } catch (XArch &e) {
    LOG((CLOG_ERR "failed to save log level setting: %s", e.what()));
  }
}

void DaemonApp::setElevate(bool elevate)
{
  LOG((CLOG_DEBUG "elevate value changed: %s", elevate ? "yes" : "no"));
  m_elevate = elevate;

  try {
    // saves setting for next time the daemon starts.
    ARCH->setting("Elevate", std::string(elevate ? "1" : "0"));
  } catch (XArch &e) {
    LOG((CLOG_ERR "failed to save elevate setting: %s", e.what()));
  }
}

void DaemonApp::setCommand(const QString &command)
{
  LOG((CLOG_INFO "service command updated"));
  m_command = command.toStdString();

  try {
    // saves setting for next time the daemon starts.
    ARCH->setting("Command", command.toStdString());
  } catch (XArch &e) {
    LOG((CLOG_ERR "failed to save command setting: %s", e.what()));
  }
}

void DaemonApp::applyWatchdogCommand()
{
  LOG((CLOG_INFO "applying watchdog command"));

#if SYSAPI_WIN32
  m_watchdog->setCommand(m_command, m_elevate);
#else
  LOG_ERR("applying watchdog command not implemented on this platform");
#endif
}

void DaemonApp::clearWatchdogCommand()
{
  LOG((CLOG_INFO "clearing watchdog command"));

#if SYSAPI_WIN32
  m_watchdog->setCommand("", false);
#else
  LOG_ERR("clearing watchdog command not implemented on this platform");
#endif
}

DaemonApp::InitResult DaemonApp::init(int argc, char **argv) // NOSONAR
{
  using enum InitResult;

  m_events = std::make_unique<EventQueue>();

  bool uninstall = false;
  try {
    // default log level to system setting.
    if (string logLevel = ARCH->setting("LogLevel"); logLevel != "")
      CLOG->setFilter(logLevel.c_str());

    for (int i = 1; i < argc; ++i) {
      string arg(argv[i]);

      if (arg == "daemon") {
        // noop
      } else if (arg == "-h" || arg == "--help") {
        const auto binName = argc > 0 ? std::filesystem::path(argv[0]).filename().string() : "deskflow-core";
        std::cout << "Usage: " << binName << " daemon [-f|--foreground] [--install] [--uninstall]" << std::endl;
        return ShowHelp;
      } else if (arg == "-f" || arg == "--foreground") {
        m_foreground = true;
      }
#if SYSAPI_WIN32
      else if (arg == "--install") {
        LOG((CLOG_NOTE "installing windows daemon"));
        uninstall = true;
        ARCH->installDaemon();
        return Installed;
      } else if (arg == "--uninstall") {
        LOG((CLOG_NOTE "uninstalling windows daemon"));
        ARCH->uninstallDaemon();
        return Uninstalled;
      }
#endif
      else {
        stringstream ss;
        ss << "Unrecognized argument: " << arg;
        handleError(ss.str().c_str());
        return ArgsError;
      }
    }

    if (!m_foreground) {
#if SYSAPI_WIN32
      // Only use MS debug outputter when the process is daemonized, since stdout won't be accessible
      // in that case, but is accessible when running in the foreground.
      CLOG->insert(new MSWindowsDebugOutputter());
#endif
    }

    return StartDaemon;

  } catch (XArch &e) {
    std::string message = e.what();
    if (uninstall && (message.find("The service has not been started") != std::string::npos)) {
      // TODO: if we're keeping this use error code instead (what is it?!).
      // HACK: this message happens intermittently, not sure where from but
      // it's quite misleading for the user. they thing something has gone
      // horribly wrong, but it's just the service manager reporting a false
      // positive (the service has actually shut down in most cases).
    } else {
      handleError(message.c_str());
    }
  } catch (std::exception &e) {
    handleError(e.what());
  } catch (...) {
    handleError("Unrecognized error.");
  }

  return FatalError;
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

    std::string command = ARCH->setting("Command");
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

void DaemonApp::handleError(const char *message)
{
  // Always print error to stdout in case run as CLI program.
  LOG_ERR("%s", message);

  // Show a message box for when run from MSI in Win32 subsystem.
#if SYSAPI_WIN32
  MessageBoxA(nullptr, message, "Deskflow daemon error", MB_OK | MB_ICONERROR);
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
    std::string command = cm->command();

    // if empty quotes, clear.
    if (command == "\"\"") {
      command.clear();
    }

    if (!command.empty()) {
      LOG((CLOG_DEBUG "daemon got new core command"));
      LOG((CLOG_DEBUG2 "new command, elevate=%d command=%s", cm->elevate(), command.c_str()));

      std::vector<std::string> argsArray;
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
      std::string logLevel(ArgParser::argsBase().m_logFilter);
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
      ARCH->setting("Elevate", std::string(cm->elevate() ? "1" : "0"));
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
    std::string type;
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
    std::string watchdogStatus = m_watchdog->isProcessActive() ? "active" : "idle";
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

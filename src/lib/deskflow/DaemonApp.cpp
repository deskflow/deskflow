/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: split this class into windows and unix to get rid
// of all the #ifdefs!

#include "deskflow/DaemonApp.h"

#include "arch/XArch.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "base/log_outputters.h"
#include "common/ipc.h"
#include "deskflow/App.h"
#include "deskflow/ArgParser.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/ServerArgs.h"
#include "ipc/DaemonIpcServer.h"
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

DaemonApp::DaemonApp(IEventQueue *events, int argc, char **argv)
    : QCoreApplication(argc, argv),
      m_events(events),
      m_ipcServer{new ipc::DaemonIpcServer(this)}
{
  connect(m_ipcServer, &ipc::DaemonIpcServer::elevateModeChanged, this, &DaemonApp::handleElevateModeChanged);
  connect(m_ipcServer, &ipc::DaemonIpcServer::commandChanged, this, &DaemonApp::handleCommandChanged);
  connect(m_ipcServer, &ipc::DaemonIpcServer::restartRequested, this, &DaemonApp::handleRestartRequested);

  s_instance = this;
}

DaemonApp::~DaemonApp()
{
  s_instance = nullptr;
}

void DaemonApp::handleElevateModeChanged(int mode)
{
  LOG_DEBUG("elevate mode changed: %d", mode);
  m_elevateMode = mode;
}

void DaemonApp::handleCommandChanged(const QString &command)
{
  LOG_DEBUG("service command updated");
  m_command = command.toStdString();
}

void DaemonApp::handleRestartRequested()
{
  LOG_DEBUG("service restart requested");
#if SYSAPI_WIN32
  m_watchdog->setCommand(m_command, m_elevateMode);
#else
  LOG_ERR("restart not implemented on this platform");
#endif
}

void DaemonApp::startAsync()
{
#if SYSAPI_WIN32
  m_watchdog->startAsync();
#endif
}

int DaemonApp::init(int argc, char **argv)
{
  bool uninstall = false;
  try {
    // default log level to system setting.
    if (string logLevel = ARCH->setting("LogLevel"); logLevel != "")
      CLOG->setFilter(logLevel.c_str());

    bool foreground = false;

    for (int i = 1; i < argc; ++i) {
      string arg(argv[i]);

      if (arg == "-f") {
        foreground = true;
      }
#if SYSAPI_WIN32
      else if (arg == "--install" || arg == "/install") {
        LOG((CLOG_NOTE "installing windows daemon"));
        uninstall = true;
        ARCH->installDaemon();
        return kExitSuccess;
      } else if (arg == "--uninstall" || arg == "/uninstall") {
        LOG((CLOG_NOTE "uninstalling windows daemon"));
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

    if (!foreground) {
#if SYSAPI_WIN32
      // Only use MS debug outputter when the process is daemonized, since stdout won't be accessible
      // in that case, but is accessible when running in the foreground.
      CLOG->insert(new MSWindowsDebugOutputter()); // NOSONAR - Adopted by `Log`
#endif
    }

    return kExitSuccess;
  } catch (XArch &e) {
    std::string message = e.what();
    if (uninstall && (message.find("The service has not been started") != std::string::npos)) {
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
      m_fileLogOutputter = new FileLogOutputter(logFilename().c_str()); // NOSONAR -- Adopted by `Log`
      CLOG->insert(m_fileLogOutputter);
    }

#if SYSAPI_WIN32
    m_watchdog = std::make_unique<MSWindowsWatchdog>(false, foreground);
    m_watchdog->setFileLogOutputter(m_fileLogOutputter);
#endif

#if SYSAPI_WIN32

    // install the platform event queue to handle service stop events.
    m_events->adoptBuffer(new MSWindowsEventQueueBuffer(m_events));

    std::string command = ARCH->setting("Command");
    bool elevate = ARCH->setting("Elevate") == "1";
    if (command != "") {
      LOG_DEBUG("using last known command: %s", command.c_str());
      m_watchdog->setCommand(command, elevate);
    }

    m_watchdog->startAsync();
#endif
    m_events->loop();

#if SYSAPI_WIN32
    m_watchdog->stop();
#endif

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
  MessageBoxA(NULL, message, "Deskflow Service", MB_OK | MB_ICONERROR);
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

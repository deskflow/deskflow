/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/Common.h"
#include "deskflow/IApp.h"
#include "net/SocketMultiplexer.h"

#if SYSAPI_WIN32
#include "deskflow/win32/AppUtilWindows.h"
#elif SYSAPI_UNIX
#include "deskflow/unix/AppUtilUnix.h"
#endif

#include <memory>
#include <stdexcept>

namespace deskflow {
class Screen;
}

class ILogOutputter;
class FileLogOutputter;
class IEventQueue;
class SocketMultiplexer;

class App : public IApp
{
public:
  class XNoEiSupport : public std::runtime_error
  {
  public:
    XNoEiSupport() : std::runtime_error("libei is not supported")
    {
      // do nothing
    }
  };

  App(IEventQueue *events, deskflow::ArgsBase *args);
  App(App const &) = delete;
  App(App &&) = delete;
  ~App() override;

  App &operator=(App const &) = delete;
  App &operator=(App &&) = delete;

  virtual void help() = 0;
  virtual void parseArgs(int argc, const char *const *argv) = 0;
  virtual void loadConfig() = 0;
  virtual bool loadConfig(const std::string &pathname) = 0;
  virtual const char *daemonInfo() const = 0;
  virtual std::string configSection() const = 0;

  virtual void version();
  void setByeFunc(void (*bye)(int)) override
  {
    m_bye = bye;
  }
  void bye(int error) override
  {
    m_bye(error);
  }
  IEventQueue *getEvents() const override
  {
    return m_events;
  }

  ARCH_APP_UTIL &appUtil()
  {
    return m_appUtil;
  }
  deskflow::ArgsBase &argsBase() const override
  {
    return *m_args;
  }
  int run(int argc, char **argv);
  int daemonMainLoop(int, const char **);
  void setupFileLogging();
  void loggingFilterWarning() const;
  void initApp(int argc, const char **argv) override;
  void initApp(int argc, char **argv)
  {
    initApp(argc, (const char **)argv);
  }
  void setEvents(EventQueue &events)
  {
    m_events = &events;
  }
  void setSocketMultiplexer(std::unique_ptr<SocketMultiplexer> &&sm)
  {
    m_socketMultiplexer = std::move(sm);
  }

  SocketMultiplexer *getSocketMultiplexer() const
  {
    return m_socketMultiplexer.get();
  }

  static App &instance()
  {
    assert(s_instance != nullptr);
    return *s_instance;
  }

  void (*m_bye)(int);

protected:
  void runEventsLoop(void *);

private:
  IEventQueue *m_events = nullptr;
  deskflow::ArgsBase *m_args;
  static App *s_instance;
  FileLogOutputter *m_fileLog = nullptr;
  ARCH_APP_UTIL m_appUtil;
  std::unique_ptr<SocketMultiplexer> m_socketMultiplexer;
};

#if WINAPI_MSWINDOWS
#define DAEMON_RUNNING(running_) ArchMiscWindows::daemonRunning(running_)
#else
#define DAEMON_RUNNING(running_)
#endif
constexpr static auto s_helpGeneralArgs = //
    "  -d, --debug <level>      filter out log messages with priority below level.\n"
    "                             level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n"
    "                             DEBUG, DEBUG1, DEBUG2.\n"
    "  -n, --name <screen-name> use screen-name instead the hostname to identify\n"
    "                             this screen in the configuration.\n"
    "  -1, --no-restart         do not try to restart on failure.\n"
    "*     --restart            restart the server automatically if it fails.\n"
    "  -l  --log <file>         write log messages to file.\n"
    "      --enable-crypto      enable TLS encryption.\n"
    "      --tls-cert           specify the path to the TLS certificate file.\n";

constexpr static auto s_helpVersionArgs = //
    "  -h, --help               display this help and exit.\n"
    "      --version            display version information and exit.\n";

constexpr static auto s_helpCommonArgs = //
    " [--name <screen-name>]"
    " [--restart|--no-restart]"
    " [--debug <level>]";

// system args (windows/unix)
#if SYSAPI_UNIX

// unix daemon mode args
constexpr static auto s_helpSysArgs = " [--daemon|--no-daemon]";
constexpr static auto s_helpSysInfo = //
    "  -f, --no-daemon          run in the foreground.\n"
    "*     --daemon             run as a daemon.\n";

#elif SYSAPI_WIN32

// windows args
constexpr static auto s_helpSysArgs = " [--service <action>] [--relaunch]";
constexpr static auto s_helpSysInfo = //
    "      --service <action>   manage the windows service, valid options are:\n"
    "                             install/uninstall/start/stop\n"
    "      --relaunch           persistently relaunches process in current user \n"
    "                             session (useful for vista and upward).\n";
#endif

#if !defined(WINAPI_LIBEI) && WINAPI_XWINDOWS
constexpr static auto s_helpNoWayland = //
    "\nYour Linux distribution does not support Wayland EI (emulated input)\n"
    "which is required for Wayland support.  Please use a Linux distribution\n"
    "that supports Wayland EI.\n";

#else
constexpr static auto s_helpNoWayland = "";
#endif

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/common.h"
#include "deskflow/IApp.h"

#if SYSAPI_WIN32
#include "deskflow/win32/AppUtilWindows.h"
#elif SYSAPI_UNIX
#include "deskflow/unix/AppUtilUnix.h"
#endif

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
    }
  };

  App(IEventQueue *events, deskflow::ArgsBase *args);
  App(App const &) = delete;
  App(App &&) = delete;
  virtual ~App();

  App &operator=(App const &) = delete;
  App &operator=(App &&) = delete;

  virtual void help() = 0;
  virtual void parseArgs(int argc, const char *const *argv) = 0;
  virtual void loadConfig() = 0;
  virtual bool loadConfig(const std::string &pathname) = 0;
  virtual const char *daemonInfo() const = 0;
  virtual std::string configSection() const = 0;

  virtual void version();
  virtual void setByeFunc(void (*bye)(int))
  {
    m_bye = bye;
  }
  virtual void bye(int error)
  {
    m_bye(error);
  }
  virtual IEventQueue *getEvents() const
  {
    return m_events;
  }

  ARCH_APP_UTIL &appUtil()
  {
    return m_appUtil;
  }
  deskflow::ArgsBase &argsBase() const
  {
    return *m_args;
  }
  int run(int argc, char **argv);
  int daemonMainLoop(int, const char **);
  void setupFileLogging();
  void loggingFilterWarning();
  void initApp(int argc, const char **argv);
  void initApp(int argc, char **argv)
  {
    initApp(argc, (const char **)argv);
  }
  void setEvents(EventQueue &events)
  {
    m_events = &events;
  }
  void setSocketMultiplexer(SocketMultiplexer *sm)
  {
    m_socketMultiplexer = sm;
  }

  SocketMultiplexer *getSocketMultiplexer() const
  {
    return m_socketMultiplexer;
  }

  static App &instance()
  {
    assert(s_instance != nullptr);
    return *s_instance;
  }

  void (*m_bye)(int);

protected:
  void runEventsLoop(void *);

  bool m_suspended;
  IEventQueue *m_events;

private:
  deskflow::ArgsBase *m_args;
  static App *s_instance;
  FileLogOutputter *m_fileLog;
  ARCH_APP_UTIL m_appUtil;
  SocketMultiplexer *m_socketMultiplexer;
};

class MinimalApp : public App
{
public:
  MinimalApp();
  virtual ~MinimalApp();

  // IApp overrides
  virtual int standardStartup(int argc, char **argv) override;
  virtual int runInner(int argc, char **argv, StartupFunc startup) override;
  virtual void startNode() override;
  virtual int mainLoop() override;
  virtual int foregroundStartup(int argc, char **argv) override;
  virtual deskflow::Screen *createScreen() override;
  virtual void loadConfig() override;
  virtual bool loadConfig(const std::string &pathname) override;
  virtual const char *daemonInfo() const override;
  virtual const char *daemonName() const override;
  virtual void parseArgs(int argc, const char *const *argv) override;

  //
  // App overrides
  //
  std::string configSection() const override
  {
    return "";
  }

private:
  Arch m_arch;
  Log m_log;
  EventQueue m_events;
};

#if WINAPI_MSWINDOWS
#define DAEMON_RUNNING(running_) ArchMiscWindows::daemonRunning(running_)
#else
#define DAEMON_RUNNING(running_)
#endif

#define HELP_COMMON_INFO_1                                                                                             \
  "  -d, --debug <level>      filter out log messages with priority below "                                            \
  "level.\n"                                                                                                           \
  "                             level may be: FATAL, ERROR, WARNING, NOTE, "                                           \
  "INFO,\n"                                                                                                            \
  "                             DEBUG, DEBUG1, DEBUG2.\n"                                                              \
  "  -n, --name <screen-name> use screen-name instead the hostname to "                                                \
  "identify\n"                                                                                                         \
  "                             this screen in the configuration.\n"                                                   \
  "  -1, --no-restart         do not try to restart on failure.\n"                                                     \
  "*     --restart            restart the server automatically if it fails.\n"                                         \
  "  -l  --log <file>         write log messages to file.\n"                                                           \
  "      --enable-drag-drop   enable file drag & drop.\n"                                                              \
  "      --enable-crypto      enable TLS encryption.\n"                                                                \
  "      --tls-cert           specify the path to the TLS certificate file.\n"

#define HELP_COMMON_INFO_2                                                                                             \
  "  -h, --help               display this help and exit.\n"                                                           \
  "      --version            display version information and exit.\n"

#define HELP_COMMON_ARGS                                                                                               \
  " [--name <screen-name>]"                                                                                            \
  " [--restart|--no-restart]"                                                                                          \
  " [--debug <level>]"

// system args (windows/unix)
#if SYSAPI_UNIX

// unix daemon mode args
#define HELP_SYS_ARGS " [--daemon|--no-daemon]"
#define HELP_SYS_INFO                                                                                                  \
  "  -f, --no-daemon          run in the foreground.\n"                                                                \
  "*     --daemon             run as a daemon.\n"

#elif SYSAPI_WIN32

// windows args
#define HELP_SYS_ARGS " [--service <action>] [--relaunch]"
#define HELP_SYS_INFO                                                                                                  \
  "      --service <action>   manage the windows service, valid options "                                              \
  "are:\n"                                                                                                             \
  "                             install/uninstall/start/stop\n"                                                        \
  "      --relaunch           persistently relaunches process in current "                                             \
  "user \n"                                                                                                            \
  "                             session (useful for vista and upward).\n"
#endif

#if !defined(WINAPI_LIBEI) && WINAPI_XWINDOWS
const auto kHelpNoWayland = "\n"
                            "Your Linux distribution does not support Wayland EI (emulated input)\n"
                            "which is required for Wayland support.  Please use a Linux distribution\n"
                            "that supports Wayland EI.\n";

#else
const auto kHelpNoWayland = "";
#endif

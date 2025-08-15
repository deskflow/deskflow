/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/App.h"

#include "DisplayInvalidException.h"
#include "VersionInfo.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/LogOutputters.h"
#include "common/Constants.h"
#include "deskflow/ArgsBase.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/ProtocolTypes.h"

#if SYSAPI_WIN32
#include "base/IEventQueue.h"
#endif

#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <vector>

#if WINAPI_CARBON
#include "platform/OSXCocoaApp.h"
#include <ApplicationServices/ApplicationServices.h>
#endif

#include <CLI/CLI.hpp>

using namespace deskflow;

App *App::s_instance = nullptr;

//
// App
//

App::App(IEventQueue *events, deskflow::ArgsBase *args)
    : m_bye(&exit),
      m_events(events),
      m_args(args),
      m_appUtil(events)
{
  assert(s_instance == nullptr);
  s_instance = this;
}

App::~App()
{
  s_instance = nullptr;
  delete m_args;
}

void App::version()
{
  const auto kBufferLength = 1024;
  std::vector<char> buffer(kBufferLength);
  std::snprintf(                                                   // NOSONAR
      buffer.data(), kBufferLength, "%s v%s, protocol v%d.%d\n%s", //
      argsBase().m_pname, kDisplayVersion, kProtocolMajorVersion, kProtocolMinorVersion, kCopyright
  );

  std::cout << std::string(buffer.data()) << std::endl;
}

int App::run(int argc, char **argv)
{
#if MAC_OS_X_VERSION_10_7
  // dock hide only supported on lion :(
  ProcessSerialNumber psn = {0, kCurrentProcess};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  GetCurrentProcess(&psn);
#pragma GCC diagnostic pop

  TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
#endif

  // install application in to arch
  appUtil().adoptApp(this);

  // HACK: fail by default (saves us setting result in each catch)
  int result = s_exitFailed;

  try {
    result = appUtil().run(argc, argv);
  } catch (ExitAppException &e) {
    // instead of showing a nasty error, just exit with the error code.
    // not sure if i like this behaviour, but it's probably better than
    // using the exit(int) function!
    result = e.getCode();
  } catch (DisplayInvalidException &die) {
    LOG_CRIT("a display invalid exception error occurred: %s\n", die.what());
    // display invalid exceptions can occur when going to sleep. When this
    // process exits, the UI will restart us instantly. We don't really want
    // that behevior, so we quies for a bit
    Arch::sleep(10);
  } catch (std::runtime_error &re) {
    LOG_CRIT("a runtime error occurred: %s\n", re.what());
  } catch (std::exception &e) {
    LOG_CRIT("an error occurred: %s\n", e.what());
  } catch (...) {
    LOG_CRIT("an unknown error occurred\n");
  }

  return result;
}

int App::daemonMainLoop(int, const char **)
{
#if SYSAPI_WIN32
  SystemLogger sysLogger(daemonName(), false);
#else
  SystemLogger sysLogger(daemonName(), true);
#endif
  return mainLoop();
}

void App::setupFileLogging()
{
  if (argsBase().m_logFile != nullptr) {
    m_fileLog = new FileLogOutputter(argsBase().m_logFile); // NOSONAR - Adopted by `Log`
    CLOG->insert(m_fileLog);
    LOG_DEBUG1("logging to file (%s) enabled", argsBase().m_logFile);
  }
}

void App::loggingFilterWarning() const
{
  if ((CLOG->getFilter() > CLOG->getConsoleMaxLevel()) && (argsBase().m_logFile == nullptr)) {
    LOG(
        (CLOG_WARN "log messages above %s are NOT sent to console (use file logging)",
         CLOG->getFilterName(CLOG->getConsoleMaxLevel()))
    );
  }
}

void App::initApp(int argc, const char **argv)
{
  CLI::App cliApp{kAppDescription};

  // Allow legacy args.
  cliApp.allow_extras();

  // Having the help argument crashes without try / catch around it
  try {
    cliApp.parse(argc, argv);
  } catch (const CLI::Error &e) {
    cliApp.exit(e);
  }

  parseArgs(argc, argv);

  // set log filter
  if (!CLOG->setFilter(argsBase().m_logFilter)) {
    LOG((
        CLOG_CRIT "%s: unrecognized log level `%s'" BYE, argsBase().m_pname, argsBase().m_logFilter, argsBase().m_pname
    ));
    m_bye(s_exitArgs);
  }
  loggingFilterWarning();

  // setup file logging after parsing args
  setupFileLogging();

  // load configuration
  loadConfig();
}

void App::handleScreenError() const
{
  LOG_CRIT("error on screen");
  getEvents()->addEvent(Event(EventTypes::Quit));
}

void App::runEventsLoop(void *)
{
  m_events->loop();

#if WINAPI_CARBON

  stopCocoaLoop();

#endif
}

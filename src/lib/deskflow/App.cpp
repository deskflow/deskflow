/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2026 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/App.h"

#include "DisplayInvalidException.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/LogOutputters.h"
#include "common/ExitCodes.h"
#include "common/Settings.h"
#include "deskflow/DeskflowException.h"
#include "mt/ThreadException.h"

#if defined(Q_OS_WIN)
#include "base/IEventQueue.h"
#endif

#include <stdexcept>

#if defined(Q_OS_MAC)
#include <ApplicationServices/ApplicationServices.h>
#endif

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
#include "platform/XDGPortalRegistry.h"
#endif

using namespace deskflow;

App *App::s_instance = nullptr;

//
// App
//

App::App(IEventQueue *events, const QString &processName)
    : m_bye(&exit),
      m_events(events),
      m_appUtil(events),
      m_pname(processName)
{
  assert(s_instance == nullptr);
  s_instance = this;
#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
  deskflow::platform::setAppId();
#endif
}

App::~App()
{
  s_instance = nullptr;
}

void App::run(QThread &coreThread)
{
  LOG_INFO("starting core");

  // Important: Move the daemon app to the daemon thread before creating any more Qt objects
  // owned by the daemon app, as they will be created on the daemon thread.
  moveToThread(&coreThread);

  connect(&coreThread, &QThread::started, this, [this, &coreThread]() {
    LOG_DEBUG("core thread started");

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
      result = appUtil().run();
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

    if (result == s_exitSuccess) {
      LOG_INFO("core stopped successfully");
    } else {
      updateExitCode(result);
      LOG_ERR("core stopped with error code: %d", result);
    }

    coreThread.quit();
    LOG_DEBUG("core thread finished");
  });

  LOG_DEBUG("starting core thread");
  coreThread.start();
}

void App::setupFileLogging()
{
  if (Settings::value(Settings::Log::ToFile).toBool()) {
    const auto file = Settings::value(Settings::Log::File).toString();
    m_fileLog = new FileLogOutputter(file); // NOSONAR - Adopted by `Log`
    CLOG->insert(m_fileLog);
    LOG_VERBOSE("logging to file (%s) enabled", qPrintable(file));
  }
}

void App::loggingFilterWarning() const
{
  if ((CLOG->getFilter() > CLOG->getConsoleMaxLevel()) && (Settings::value(Settings::Log::ToFile).toBool())) {
    LOG_WARN(
        "log messages above %s are NOT sent to console (use file logging)",
        qPrintable(LogLevel::toOption(CLOG->getConsoleMaxLevel()))
    );
  }
}

void App::initApp()
{
  parseArgs();

  // set log filter
  if (const auto logLevel = Settings::logLevelText(); !CLOG->setFilter(logLevel)) {
    LOG_CRIT(
        "%s: unrecognized log level `%s'" BYE, qPrintable(processName()), qPrintable(logLevel),
        qPrintable(processName())
    );
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

void App::quit() const
{
  LOG_INFO("quitting");
  getEvents()->addEvent(Event(EventTypes::Quit));
}

void App::runEventsLoop(const void *)
{
  int exitCode = m_events->loop();
  if (exitCode != s_exitSuccess) {
    throw ThreadExitException(new LoopErrorCode(exitCode));
  }
}

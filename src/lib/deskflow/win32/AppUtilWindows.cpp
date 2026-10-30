/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/win32/AppUtilWindows.h"
#include "deskflow/KeyboardLayoutManager.h"

#include "arch/Arch.h"
#include "arch/win32/ArchDaemonWindows.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/LogOutputters.h"
#include "common/Constants.h"
#include "deskflow/App.h"
#include "deskflow/Computer.h"
#include "deskflow/DeskflowException.h"
#include "mt/Thread.h"
#include "platform/MSWindowsComputer.h"

#include <Windows.h>
#include <conio.h>

namespace {

std::string languageForKeyboardLayout(HKL layout)
{
  if (!layout) {
    LOG_WARN("Cannot read keyboard language: null keyboard layout");
    return {};
  }
  const auto locale = MAKELCID(LOWORD(reinterpret_cast<ULONG_PTR>(layout)), SORT_DEFAULT);
  // LOCALE_SISO639LANGNAME allows up to nine characters, including the NUL.
  std::string code(9, '\0');
  const int written = GetLocaleInfoA(locale, LOCALE_SISO639LANGNAME, code.data(), static_cast<int>(code.size()));
  if (written == 0) {
    const auto error = GetLastError();
    LOG_WARN(
        "Cannot read keyboard language for locale %lu: Windows error %lu", static_cast<unsigned long>(locale),
        static_cast<unsigned long>(error)
    );
    return {};
  }
  if (written == 1) {
    LOG_WARN("Windows returned an empty keyboard language for locale %lu", static_cast<unsigned long>(locale));
    return {};
  }
  code.resize(written - 1);
  return deskflow::KeyboardLayoutManager::languageForISO639_1(code);
}

} // namespace

AppUtilWindows::AppUtilWindows(IEventQueue *events) : m_events(events), m_exitMode(kExitModeNormal)
{
  if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE) == FALSE) {
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  m_eventThread = std::thread(&AppUtilWindows::eventLoop, this); // NOSONAR - No jthread on Windows

  // Waiting for the event loop start prevents race condition in fast fail scenario,
  // where the dtor is called just before the event loop starts.
  LOG_DEBUG("waiting for event thread to start");
  std::unique_lock lock(m_eventThreadStartedMutex);
  m_eventThreadStartedCond.wait(lock, [this] { return m_eventThreadRunning; });
  LOG_DEBUG("event thread started");
}

AppUtilWindows::~AppUtilWindows()
{
  m_eventThreadRunning = false;
  m_eventThread.join();
}

BOOL WINAPI AppUtilWindows::consoleHandler(DWORD)
{
  LOG_INFO("got shutdown signal");
  IEventQueue *events = AppUtil::instance().app().getEvents();
  events->addEvent(Event(EventTypes::Quit));
  return TRUE;
}

static int mainLoopStatic()
{
  return AppUtil::instance().app().mainLoop();
}

int AppUtilWindows::daemonNTMainLoop()
{
  app().initApp();

  return ArchDaemonWindows::runDaemon(mainLoopStatic);
}

void AppUtilWindows::exitApp(int code)
{
  switch (m_exitMode) {

  case kExitModeDaemon:
    ArchDaemonWindows::daemonFailed(code);
    break;

  default:
    throw ExitAppException(code);
  }
}

int daemonNTMainLoopStatic()
{
  return AppUtilWindows::instance().daemonNTMainLoop();
}

int AppUtilWindows::daemonNTStartup()
{
  SystemLogger sysLogger(app().daemonName(), false);
  m_exitMode = kExitModeDaemon;
  return ARCH->daemonize(daemonNTMainLoopStatic);
}

static int daemonNTStartupStatic()
{
  return AppUtilWindows::instance().daemonNTStartup();
}

static int foregroundStartupStatic()
{
  return AppUtil::instance().app().start();
}

int AppUtilWindows::run()
{
  // record window instance for tray icon, etc
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));

  MSWindowsComputer::init(ArchMiscWindows::instanceWin32());
  Thread::getCurrentThread().setPriority(-14);

  StartupFunc startup;
  if (ArchMiscWindows::wasLaunchedAsService()) {
    startup = &daemonNTStartupStatic;
  } else {
    startup = &foregroundStartupStatic;
  }

  return app().runInner(startup);
}

AppUtilWindows &AppUtilWindows::instance()
{
  return (AppUtilWindows &)AppUtil::instance();
}

std::vector<std::string> AppUtilWindows::getKeyboardLayoutList()
{
  std::vector<std::string> layoutLangCodes;
  {
    auto uLayouts = GetKeyboardLayoutList(0, nullptr);
    auto lpList = (HKL *)LocalAlloc(LPTR, (uLayouts * sizeof(HKL)));
    uLayouts = GetKeyboardLayoutList(uLayouts, lpList);

    for (int i = 0; i < uLayouts; ++i) {
      // Preserve the layout index even when its language cannot be represented.
      layoutLangCodes.push_back(languageForKeyboardLayout(lpList[i]));
    }

    if (lpList) {
      LocalFree(lpList);
    }
  }
  return layoutLangCodes;
}

std::string AppUtilWindows::getCurrentLanguageCode()
{
  return languageForKeyboardLayout(getCurrentKeyboardLayout());
}

HKL AppUtilWindows::getCurrentKeyboardLayout() const
{
  HKL layout = nullptr;

  GUITHREADINFO gti = {sizeof(GUITHREADINFO)};
  if (GetGUIThreadInfo(0, &gti) && gti.hwndActive) {
    layout = GetKeyboardLayout(GetWindowThreadProcessId(gti.hwndActive, nullptr));
  } else {
    LOG_WARN("failed to determine current keyboard layout");
  }

  return layout;
}

void AppUtilWindows::eventLoop()
{
  HANDLE hCloseEvent = CreateEvent(nullptr, TRUE, FALSE, kCloseEventName);
  if (!hCloseEvent) {
    LOG_CRIT("failed to create event for windows event loop");
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  LOG_DEBUG("windows event loop running");
  {
    std::scoped_lock lock{m_eventThreadStartedMutex};
    m_eventThreadRunning = true;
  }
  m_eventThreadStartedCond.notify_one();

  while (m_eventThreadRunning) {
    // Wait for 100ms at most so that we can stop the loop when the app is closing, if not already stopped.
    DWORD closeEventResult = MsgWaitForMultipleObjects(1, &hCloseEvent, FALSE, 100, QS_ALLINPUT);

    if (closeEventResult == WAIT_OBJECT_0) {
      LOG_DEBUG("windows event loop received close event");
      m_events->addEvent(Event(EventTypes::Quit));
      m_eventThreadRunning = false;
    } else if (closeEventResult == WAIT_OBJECT_0 + 1) {
      MSG msg;
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  CloseHandle(hCloseEvent);
  LOG_DEBUG("windows event loop finished");
}

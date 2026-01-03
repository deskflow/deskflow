/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/win32/AppUtilWindows.h"

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
#include "deskflow/DeskflowException.h"
#include "deskflow/Screen.h"
#include "mt/Thread.h"
#include "platform/MSWindowsScreen.h"

#include <Windows.h>
#include <conio.h>

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

  MSWindowsScreen::init(ArchMiscWindows::instanceWin32());
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

void AppUtilWindows::startNode()
{
  app().startNode();
}

std::vector<std::string> AppUtilWindows::getKeyboardLayoutList()
{
  std::vector<std::string> layoutLangCodes;
  {
    auto uLayouts = GetKeyboardLayoutList(0, nullptr);
    auto lpList = (HKL *)LocalAlloc(LPTR, (uLayouts * sizeof(HKL)));
    uLayouts = GetKeyboardLayoutList(uLayouts, lpList);

    for (int i = 0; i < uLayouts; ++i) {
      std::string code("", 2);
      GetLocaleInfoA(
          MAKELCID(((ULONG_PTR)lpList[i] & 0xffffffff), SORT_DEFAULT), LOCALE_SISO639LANGNAME, &code[0],
          static_cast<int>(code.size())
      );
      layoutLangCodes.push_back(code);
    }

    if (lpList) {
      LocalFree(lpList);
    }
  }
  return layoutLangCodes;
}

std::string AppUtilWindows::getCurrentLanguageCode()
{
  std::string code("", 2);

  auto hklLayout = getCurrentKeyboardLayout();
  if (hklLayout) {
    auto localLayoutID = MAKELCID(LOWORD(hklLayout), SORT_DEFAULT);
    GetLocaleInfoA(localLayoutID, LOCALE_SISO639LANGNAME, &code[0], static_cast<int>(code.size()));
  }

  return code;
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

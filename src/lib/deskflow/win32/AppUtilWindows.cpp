/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "deskflow/win32/AppUtilWindows.h"

#include "arch/IArchTaskBarReceiver.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"
#include "base/Event.h"
#include "base/EventQueue.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/log_outputters.h"
#include "common/constants.h"
#include "deskflow/App.h"
#include "deskflow/ArgsBase.h"
#include "deskflow/Screen.h"
#include "deskflow/XDeskflow.h"
#include "platform/MSWindowsScreen.h"

#include <VersionHelpers.h>
#include <Windows.h>
#include <conio.h>
#include <iostream>
#include <memory>
#include <sstream>

#if HAVE_WINTOAST
#include "wintoastlib.h"
#endif

AppUtilWindows::AppUtilWindows(IEventQueue *events) : m_events(events), m_exitMode(kExitModeNormal)
{
  if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE) == FALSE) {
    throw XArch(new XArchEvalWindows());
  }
}

AppUtilWindows::~AppUtilWindows()
{
}

BOOL WINAPI AppUtilWindows::consoleHandler(DWORD)
{
  LOG((CLOG_INFO "got shutdown signal"));
  IEventQueue *events = AppUtil::instance().app().getEvents();
  events->addEvent(Event(Event::kQuit));
  return TRUE;
}

static int mainLoopStatic()
{
  return AppUtil::instance().app().mainLoop();
}

int AppUtilWindows::daemonNTMainLoop(int argc, const char **argv)
{
  app().initApp(argc, argv);
  debugServiceWait();

  return ArchMiscWindows::runDaemon(mainLoopStatic);
}

void AppUtilWindows::exitApp(int code)
{
  switch (m_exitMode) {

  case kExitModeDaemon:
    ArchMiscWindows::daemonFailed(code);
    break;

  default:
    throw XExitApp(code);
  }
}

int daemonNTMainLoopStatic(int argc, const char **argv)
{
  return AppUtilWindows::instance().daemonNTMainLoop(argc, argv);
}

int AppUtilWindows::daemonNTStartup(int, char **)
{
  SystemLogger sysLogger(app().daemonName(), false);
  m_exitMode = kExitModeDaemon;
  return ARCH->daemonize(app().daemonName(), daemonNTMainLoopStatic);
}

static int daemonNTStartupStatic(int argc, char **argv)
{
  return AppUtilWindows::instance().daemonNTStartup(argc, argv);
}

static int foregroundStartupStatic(int argc, char **argv)
{
  return AppUtil::instance().app().foregroundStartup(argc, argv);
}

void AppUtilWindows::beforeAppExit()
{
  // this can be handy for debugging, since the application is launched in
  // a new console window, and will normally close on exit (making it so
  // that we can't see error messages).
  if (app().argsBase().m_pauseOnExit) {
    std::cout << std::endl << "press any key to exit..." << std::endl;
    int c = _getch();
  }
}

int AppUtilWindows::run(int argc, char **argv)
{
  if (!IsWindowsXPSP3OrGreater()) {
    throw std::runtime_error("unsupported os version, xp sp3 or greater required");
  }

  // record window instance for tray icon, etc
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));

  MSWindowsScreen::init(ArchMiscWindows::instanceWin32());
  Thread::getCurrentThread().setPriority(-14);

  StartupFunc startup;
  if (ArchMiscWindows::wasLaunchedAsService()) {
    startup = &daemonNTStartupStatic;
  } else {
    startup = &foregroundStartupStatic;
    app().argsBase().m_daemon = false;
  }

  return app().runInner(argc, argv, NULL, startup);
}

AppUtilWindows &AppUtilWindows::instance()
{
  return (AppUtilWindows &)AppUtil::instance();
}

void AppUtilWindows::debugServiceWait()
{
  if (app().argsBase().m_debugServiceWait) {
    while (true) {
      // this code is only executed when the process is launched via the
      // windows service controller (and --debug-service-wait arg is
      // used). to debug, set a breakpoint on this line so that
      // execution is delayed until the debugger is attached.
      ARCH->sleep(1);
      LOG((CLOG_INFO "waiting for debugger to attach"));
    }
  }
}

void AppUtilWindows::startNode()
{
  app().startNode();
}

std::vector<String> AppUtilWindows::getKeyboardLayoutList()
{
  std::vector<String> layoutLangCodes;
  {
    auto uLayouts = GetKeyboardLayoutList(0, NULL);
    auto lpList = (HKL *)LocalAlloc(LPTR, (uLayouts * sizeof(HKL)));
    uLayouts = GetKeyboardLayoutList(uLayouts, lpList);

    for (int i = 0; i < uLayouts; ++i) {
      String code("", 2);
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

String AppUtilWindows::getCurrentLanguageCode()
{
  String code("", 2);

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
    layout = GetKeyboardLayout(GetWindowThreadProcessId(gti.hwndActive, NULL));
  } else {
    LOG((CLOG_WARN "failed to determine current keyboard layout"));
  }

  return layout;
}

#if HAVE_WINTOAST
class WinToastHandler : public WinToastLib::IWinToastHandler
{
public:
  WinToastHandler()
  {
  }
  // Public interfaces
  void toastActivated() const override
  {
  }
  void toastActivated(int actionIndex) const override
  {
  }
  void toastDismissed(WinToastDismissalReason state) const override
  {
  }
  void toastFailed() const override
  {
  }
};
#endif

void AppUtilWindows::showNotification(const String &title, const String &text) const
{
#if HAVE_WINTOAST
  LOG((CLOG_INFO "showing notification, title=\"%s\", text=\"%s\"", title.c_str(), text.c_str()));
  if (!WinToastLib::WinToast::isCompatible()) {
    LOG((CLOG_INFO "this system does not support toast notifications"));
    return;
  }
  if (!WinToastLib::WinToast::instance()->isInitialized()) {
    WinToastLib::WinToast::instance()->setAppName(L""
                                                  "Deskflow");
    const auto aumi = WinToastLib::WinToast::configureAUMI(
        L""
        "Deskflow",
        L""
        "Deskflow",
        L""
        "Deskflow",
        L"1.14.1+"
    );
    WinToastLib::WinToast::instance()->setAppUserModelId(aumi);

    if (!WinToastLib::WinToast::instance()->initialize()) {
      LOG((CLOG_DEBUG "failed to initialize toast notifications"));
      return;
    }
  }

  WinToastLib::WinToast::WinToastError error;
  auto handler = std::make_unique<WinToastHandler>();
  WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
  templ.setTextField(std::wstring(title.begin(), title.end()), WinToastLib::WinToastTemplate::FirstLine);
  templ.setTextField(std::wstring(text.begin(), text.end()), WinToastLib::WinToastTemplate::SecondLine);

  const bool launched = WinToastLib::WinToast::instance()->showToast(templ, handler.get(), &error);
  if (!launched) {
    LOG((CLOG_DEBUG "failed to show toast notification, error code: %d", error));
    return;
  }
#else
  LOG((CLOG_INFO "toast notifications are not supported"));
#endif
}

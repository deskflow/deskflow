/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsScreenSaver.h"

#include "arch/Arch.h"
#include "arch/win32/ArchMiscWindows.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "mt/Thread.h"
#include "platform/MSWindowsScreen.h"

#include <malloc.h>
#include <tchar.h>

#if !defined(SPI_GETSCREENSAVERRUNNING)
#define SPI_GETSCREENSAVERRUNNING 114
#endif

static const TCHAR *g_isSecureNT = L"ScreenSaverIsSecure";
static const TCHAR *g_isSecure9x = L"ScreenSaveUsePassword";
static const TCHAR *const g_pathScreenSaverIsSecure[] = {L"Control Panel", L"Desktop", nullptr};

//
// MSWindowsScreenSaver
//

MSWindowsScreenSaver::MSWindowsScreenSaver()
{
  // check if screen saver is enabled
  SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_wasEnabled, 0);
}

MSWindowsScreenSaver::~MSWindowsScreenSaver()
{
  unwatchProcess();
}

bool MSWindowsScreenSaver::checkStarted(UINT msg, WPARAM wParam, LPARAM lParam)
{
  // if already started then say it didn't just start
  if (m_active) {
    return false;
  }

  // screen saver may have started.  look for it and get
  // the process.  if we can't find it then assume it
  // didn't really start.  we wait a moment before
  // looking to give the screen saver a chance to start.
  // this shouldn't be a problem since we only get here
  // if the screen saver wants to kick in, meaning that
  // the system is idle or the user deliberately started
  // the screen saver.
  Sleep(250);

  // set parameters common to all screen saver handling
  m_threadID = GetCurrentThreadId();
  m_msg = msg;
  m_wParam = wParam;
  m_lParam = lParam;

  // on the windows nt family we wait for the desktop to
  // change until it's neither the Screen-Saver desktop
  // nor a desktop we can't open (the login desktop).
  // since windows will send the request-to-start-screen-
  // saver message even when the screen saver is disabled
  // we first check that the screen saver is indeed active
  // before watching for it to stop.
  if (!isActive()) {
    LOG_DEBUG2("can't open screen saver desktop");
    return false;
  }

  watchDesktop();
  return true;
}

void MSWindowsScreenSaver::enable()
{
  SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, m_wasEnabled, 0, 0);

  // restore password protection
  if (m_wasSecure) {
    setSecure(true, m_wasSecureAnInt);
  }
}

void MSWindowsScreenSaver::disable()
{
  SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_wasEnabled, 0);
  SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, 0, 0);

  // disable password protected screensaver
  m_wasSecure = isSecure(&m_wasSecureAnInt);
  if (m_wasSecure) {
    setSecure(false, m_wasSecureAnInt);
  }
}

void MSWindowsScreenSaver::activate()
{
  // don't activate if already active
  if (!isActive()) {
    // activate
    HWND hwnd = GetForegroundWindow();
    if (hwnd != nullptr) {
      PostMessage(hwnd, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
    } else {
      // no foreground window.  pretend we got the event instead.
      DefWindowProc(nullptr, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
    }
  }
}

void MSWindowsScreenSaver::deactivate()
{
  bool killed = false;

  // NT runs screen saver in another desktop
  HDESK desktop = OpenDesktop(L"Screen-saver", 0, FALSE, DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS);
  if (desktop != nullptr) {
    EnumDesktopWindows(desktop, &MSWindowsScreenSaver::killScreenSaverFunc, reinterpret_cast<LPARAM>(&killed));
    CloseDesktop(desktop);
  }

  // if above failed or wasn't tried, try the windows 95 way
  if (!killed) {
    // find screen saver window and close it
    HWND hwnd = FindWindow(L"WindowsScreenSaverClass", nullptr);
    if (hwnd == nullptr) {
      // win2k may use a different class
      hwnd = FindWindow(L"Default Screen Saver", nullptr);
    }
    if (hwnd != nullptr) {
      PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
  }

  // force timer to restart
  SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_wasEnabled, 0);
  SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, !m_wasEnabled, 0, SPIF_SENDWININICHANGE);
  SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, m_wasEnabled, 0, SPIF_SENDWININICHANGE);
}

bool MSWindowsScreenSaver::isActive() const
{
  BOOL running;
  SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &running, 0);
  return (running != FALSE);
}

BOOL CALLBACK MSWindowsScreenSaver::killScreenSaverFunc(HWND hwnd, LPARAM arg)
{
  if (IsWindowVisible(hwnd)) {
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    if (instance != MSWindowsScreen::getWindowInstance()) {
      PostMessage(hwnd, WM_CLOSE, 0, 0);
      *reinterpret_cast<bool *>(arg) = true;
    }
  }
  return TRUE;
}

void MSWindowsScreenSaver::watchDesktop()
{
  // stop watching previous process/desktop
  unwatchProcess();

  // watch desktop in another thread
  LOG_DEBUG("watching screen saver desktop");
  m_active = true;
  m_watch = new Thread(new TMethodJob<MSWindowsScreenSaver>(this, &MSWindowsScreenSaver::watchDesktopThread));
}

void MSWindowsScreenSaver::watchProcess(HANDLE process)
{
  // stop watching previous process/desktop
  unwatchProcess();

  // watch new process in another thread
  if (process != nullptr) {
    LOG_DEBUG("watching screen saver process");
    m_process = process;
    m_active = true;
    m_watch = new Thread(new TMethodJob<MSWindowsScreenSaver>(this, &MSWindowsScreenSaver::watchProcessThread));
  }
}

void MSWindowsScreenSaver::unwatchProcess()
{
  if (m_watch != nullptr) {
    LOG_DEBUG("stopped watching screen saver process/desktop");
    m_watch->cancel();
    m_watch->wait();
    delete m_watch;
    m_watch = nullptr;
    m_active = false;
  }
  if (m_process != nullptr) {
    CloseHandle(m_process);
    m_process = nullptr;
  }
}

void MSWindowsScreenSaver::watchDesktopThread(void *)
{
  DWORD reserved = 0;
  TCHAR *name = nullptr;

  for (;;) {
    // wait a bit
    Arch::sleep(0.2);

    BOOL running;
    SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &running, 0);
    if (running) {
      continue;
    }

    // send screen saver deactivation message
    m_active = false;
    PostThreadMessage(m_threadID, m_msg, m_wParam, m_lParam);
    return;
  }
}

void MSWindowsScreenSaver::watchProcessThread(void *)
{
  for (;;) {
    Thread::testCancel();
    if (WaitForSingleObject(m_process, 50) == WAIT_OBJECT_0) {
      // process terminated
      LOG_DEBUG("screen saver died");

      // send screen saver deactivation message
      m_active = false;
      PostThreadMessage(m_threadID, m_msg, m_wParam, m_lParam);
      return;
    }
  }
}

void MSWindowsScreenSaver::setSecure(bool secure, bool saveSecureAsInt)
{
  HKEY hkey = ArchMiscWindows::addKey(HKEY_CURRENT_USER, g_pathScreenSaverIsSecure);
  if (hkey == nullptr) {
    return;
  }

  if (saveSecureAsInt) {
    ArchMiscWindows::setValue(hkey, g_isSecureNT, secure ? 1 : 0);
  } else {
    ArchMiscWindows::setValue(hkey, g_isSecureNT, secure ? "1" : "0");
  }

  ArchMiscWindows::closeKey(hkey);
}

bool MSWindowsScreenSaver::isSecure(bool *wasSecureFlagAnInt) const
{
  // get the password protection setting key
  HKEY hkey = ArchMiscWindows::openKey(HKEY_CURRENT_USER, g_pathScreenSaverIsSecure);
  if (hkey == nullptr) {
    return false;
  }

  // get the value.  the value may be an int or a string, depending
  // on the version of windows.
  bool result;
  switch (ArchMiscWindows::typeOfValue(hkey, g_isSecureNT)) {
  default:
    result = false;
    break;

  case ArchMiscWindows::kUINT: {
    DWORD value = ArchMiscWindows::readValueInt(hkey, g_isSecureNT);
    *wasSecureFlagAnInt = true;
    result = (value != 0);
    break;
  }

  case ArchMiscWindows::kSTRING: {
    std::wstring value = ArchMiscWindows::readValueString(hkey, g_isSecureNT);
    *wasSecureFlagAnInt = false;
    result = (value != L"0");
    break;
  }
  }

  ArchMiscWindows::closeKey(hkey);
  return result;
}

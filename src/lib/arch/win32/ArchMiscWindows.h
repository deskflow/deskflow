/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Tlhelp32.h>

#include <functional>

//! Miscellaneous win32 functions.
class ArchMiscWindows
{
public:
  enum EValueType
  {
    kUNKNOWN,
    kNO_VALUE,
    kUINT,
    kSTRING,
    kBINARY
  };
  enum EBusyModes
  {
    kIDLE = 0x0000,
    kSYSTEM = 0x0001,
    kDISPLAY = 0x0002
  };

  using RunFunc = std::function<int(void)>;

  //! Initialize
  static void init();

  //! Run the daemon
  /*!
  Delegates to ArchDaemonWindows.
  */
  static int runDaemon(RunFunc runFunc);

  //! Indicate daemon is in main loop
  /*!
  Delegates to ArchDaemonWindows.
  */
  static void daemonRunning(bool running);

  //! Indicate failure of running daemon
  /*!
  Delegates to ArchDaemonWindows.
  */
  static void daemonFailed(int result);

  //! Get daemon quit message
  /*!
  Delegates to ArchDaemonWindows.
  */
  static UINT getDaemonQuitMessage();

  //! Open and return a registry key, closing the parent key
  static HKEY openKey(HKEY parent, const TCHAR *child);

  //! Open and return a registry key, closing the parent key
  static HKEY openKey(HKEY parent, const TCHAR *const *keyPath);

  //! Open/create and return a registry key, closing the parent key
  static HKEY addKey(HKEY parent, const TCHAR *child);

  //! Open/create and return a registry key, closing the parent key
  static HKEY addKey(HKEY parent, const TCHAR *const *keyPath);

  //! Close a key
  static void closeKey(HKEY);

  //! Delete a key (which should have no subkeys)
  static void deleteKey(HKEY parent, const TCHAR *name);

  //! Get type of value
  static EValueType typeOfValue(HKEY key, const TCHAR *name);

  //! Set a string value in the registry
  static void setValue(HKEY key, const TCHAR *name, const std::string &value);

  //! Set a DWORD value in the registry
  static void setValue(HKEY key, const TCHAR *name, DWORD value);

  //! Read a string value from the registry
  static std::wstring readValueString(HKEY, const TCHAR *name);

  //! Read a DWORD value from the registry
  static DWORD readValueInt(HKEY, const TCHAR *name);

  //! Disable power saving
  static void addBusyState(DWORD busyModes);

  //! Enable power saving
  static void removeBusyState(DWORD busyModes);

  //! Briefly interrupt power saving
  static void wakeupDisplay();

  //! Returns true if this process was launched via NT service host.
  static bool wasLaunchedAsService();

  //! Returns true if we got the parent process name.
  static bool getParentProcessName(std::wstring &name);

  //! Prevent hard to troubleshoot errors, e.g. access violations.
  static void guardRuntimeVersion();

  //! Gets the window instance saved at program start.
  /*!
  e.g. Used by `GetModuleFileName` which is used when installing the daemon.
  */
  static HINSTANCE instanceWin32();

  //! Saves the window instance for later use.
  static void setInstanceWin32(HINSTANCE instance);

  //! Get the name of the active input desktop.
  static std::wstring getActiveDesktopName();

  //! Returns true if the process is running with elevated privileges (i.e. as admin).
  static bool isProcessElevated();

private:
  //! Open and return a registry key, closing the parent key
  static HKEY openKey(HKEY parent, const TCHAR *child, bool create);

  //! Open and return a registry key, closing the parent key
  static HKEY openKey(HKEY parent, const TCHAR *const *keyPath, bool create);

  //! Read a string value from the registry
  static std::wstring readBinaryOrString(HKEY, const TCHAR *name, DWORD type);

  //! Set thread busy state
  static void setThreadExecutionState(DWORD);

  //! Dummy function for thread execution state
  static DWORD WINAPI dummySetThreadExecutionState(DWORD);

  //! Iterates over the process snapshot to find a process entry
  static BOOL WINAPI getProcessEntry(PROCESSENTRY32 &entry, DWORD processID);

  //! Calls `getProcessEntry` with the current process ID
  static BOOL WINAPI getSelfProcessEntry(PROCESSENTRY32 &entry);

  //! Calls `getProcessEntry` with the parent process ID
  static BOOL WINAPI getParentProcessEntry(PROCESSENTRY32 &entry);

  //! Searches the loaded modules and returns the matching module handle
  /**
   * @param moduleNames Provide two module names to search for both release and debug versions.
   */
  static HMODULE findLoadedModule(std::array<const char *, 2> moduleNames);

private:
  typedef DWORD(WINAPI *STES_t)(DWORD);

  static DWORD s_busyState;
  static STES_t s_stes; // STES: Set thread execution state
  static HICON s_largeIcon;
  static HICON s_smallIcon;
  static HINSTANCE s_instanceWin32;
};

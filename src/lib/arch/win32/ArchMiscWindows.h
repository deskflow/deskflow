/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#pragma once

#include "common/common.h"
#include "common/stdstring.h"
#include "common/stdset.h"
#include "base/String.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Tlhelp32.h>

//! Miscellaneous win32 functions.
class ArchMiscWindows {
public:
    enum EValueType { kUNKNOWN, kNO_VALUE, kUINT, kSTRING, kBINARY };
    enum EBusyModes { kIDLE = 0x0000, kSYSTEM = 0x0001, kDISPLAY = 0x0002 };

    typedef int (*RunFunc) (void);

    //! Initialize
    static void init ();

    //! Delete memory
    static void cleanup ();

    //! Set the application icons
    /*!
    Set the application icons.
    */
    static void setIcons (HICON largeIcon, HICON smallIcon);

    //! Get the application icons
    /*!
    Get the application icons.
    */
    static void getIcons (HICON& largeIcon, HICON& smallIcon);

    //! Run the daemon
    /*!
    Delegates to ArchDaemonWindows.
    */
    static int runDaemon (RunFunc runFunc);

    //! Indicate daemon is in main loop
    /*!
    Delegates to ArchDaemonWindows.
    */
    static void daemonRunning (bool running);

    //! Indicate failure of running daemon
    /*!
    Delegates to ArchDaemonWindows.
    */
    static void daemonFailed (int result);

    //! Get daemon quit message
    /*!
    Delegates to ArchDaemonWindows.
    */
    static UINT getDaemonQuitMessage ();

    //! Open and return a registry key, closing the parent key
    static HKEY openKey (HKEY parent, const TCHAR* child);

    //! Open and return a registry key, closing the parent key
    static HKEY openKey (HKEY parent, const TCHAR* const* keyPath);

    //! Open/create and return a registry key, closing the parent key
    static HKEY addKey (HKEY parent, const TCHAR* child);

    //! Open/create and return a registry key, closing the parent key
    static HKEY addKey (HKEY parent, const TCHAR* const* keyPath);

    //! Close a key
    static void closeKey (HKEY);

    //! Delete a key (which should have no subkeys)
    static void deleteKey (HKEY parent, const TCHAR* name);

    //! Delete a value
    static void deleteValue (HKEY parent, const TCHAR* name);

    //! Test if a value exists
    static bool hasValue (HKEY key, const TCHAR* name);

    //! Get type of value
    static EValueType typeOfValue (HKEY key, const TCHAR* name);

    //! Set a string value in the registry
    static void
    setValue (HKEY key, const TCHAR* name, const std::string& value);

    //! Set a DWORD value in the registry
    static void setValue (HKEY key, const TCHAR* name, DWORD value);

    //! Set a BINARY value in the registry
    /*!
    Sets the \p name value of \p key to \p value.data().
    */
    static void
    setValueBinary (HKEY key, const TCHAR* name, const std::string& value);

    //! Read a string value from the registry
    static std::string readValueString (HKEY, const TCHAR* name);

    //! Read a DWORD value from the registry
    static DWORD readValueInt (HKEY, const TCHAR* name);

    //! Read a BINARY value from the registry
    static std::string readValueBinary (HKEY, const TCHAR* name);

    //! Add a dialog
    static void addDialog (HWND);

    //! Remove a dialog
    static void removeDialog (HWND);

    //! Process dialog message
    /*!
    Checks if the message is destined for a dialog.  If so the message
    is passed to the dialog and returns true, otherwise returns false.
    */
    static bool processDialog (MSG*);

    //! Disable power saving
    static void addBusyState (DWORD busyModes);

    //! Enable power saving
    static void removeBusyState (DWORD busyModes);

    //! Briefly interrupt power saving
    static void wakeupDisplay ();

    //! Returns true if this process was launched via NT service host.
    static bool wasLaunchedAsService ();

    //! Returns true if we got the parent process name.
    static bool getParentProcessName (String& name);

    static HINSTANCE instanceWin32 ();

    static void setInstanceWin32 (HINSTANCE instance);

    static BOOL WINAPI getProcessEntry (PROCESSENTRY32& entry, DWORD processID);
    static BOOL WINAPI getSelfProcessEntry (PROCESSENTRY32& entry);
    static BOOL WINAPI getParentProcessEntry (PROCESSENTRY32& entry);

private:
    //! Open and return a registry key, closing the parent key
    static HKEY openKey (HKEY parent, const TCHAR* child, bool create);

    //! Open and return a registry key, closing the parent key
    static HKEY openKey (HKEY parent, const TCHAR* const* keyPath, bool create);

    //! Read a string value from the registry
    static std::string readBinaryOrString (HKEY, const TCHAR* name, DWORD type);

    //! Set thread busy state
    static void setThreadExecutionState (DWORD);

    static DWORD WINAPI dummySetThreadExecutionState (DWORD);

private:
    typedef std::set<HWND> Dialogs;
    typedef DWORD (WINAPI* STES_t) (DWORD);

    static Dialogs* s_dialogs;
    static DWORD s_busyState;
    static STES_t s_stes;
    static HICON s_largeIcon;
    static HICON s_smallIcon;
    static HINSTANCE s_instanceWin32;
};

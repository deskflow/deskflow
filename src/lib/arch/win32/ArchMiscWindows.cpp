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

#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/ArchDaemonWindows.h"
#include "base/Log.h"
#include "common/Version.h"

#include <Wtsapi32.h>
#pragma warning(disable : 4099)
#include <Userenv.h>
#pragma warning(default : 4099)

// parent process name for services in Vista
#define SERVICE_LAUNCHER "services.exe"

#ifndef ES_SYSTEM_REQUIRED
#define ES_SYSTEM_REQUIRED ((DWORD) 0x00000001)
#endif
#ifndef ES_DISPLAY_REQUIRED
#define ES_DISPLAY_REQUIRED ((DWORD) 0x00000002)
#endif
#ifndef ES_CONTINUOUS
#define ES_CONTINUOUS ((DWORD) 0x80000000)
#endif
typedef DWORD EXECUTION_STATE;

//
// ArchMiscWindows
//

ArchMiscWindows::Dialogs* ArchMiscWindows::s_dialogs = NULL;
DWORD ArchMiscWindows::s_busyState                   = 0;
ArchMiscWindows::STES_t ArchMiscWindows::s_stes      = NULL;
HICON ArchMiscWindows::s_largeIcon                   = NULL;
HICON ArchMiscWindows::s_smallIcon                   = NULL;
HINSTANCE ArchMiscWindows::s_instanceWin32           = NULL;

void
ArchMiscWindows::cleanup () {
    delete s_dialogs;
}

void
ArchMiscWindows::init () {
    // stop windows system error dialogs from showing.
    SetErrorMode (SEM_FAILCRITICALERRORS);

    s_dialogs = new Dialogs;
}

void
ArchMiscWindows::setIcons (HICON largeIcon, HICON smallIcon) {
    s_largeIcon = largeIcon;
    s_smallIcon = smallIcon;
}

void
ArchMiscWindows::getIcons (HICON& largeIcon, HICON& smallIcon) {
    largeIcon = s_largeIcon;
    smallIcon = s_smallIcon;
}

int
ArchMiscWindows::runDaemon (RunFunc runFunc) {
    return ArchDaemonWindows::runDaemon (runFunc);
}

void
ArchMiscWindows::daemonRunning (bool running) {
    ArchDaemonWindows::daemonRunning (running);
}

void
ArchMiscWindows::daemonFailed (int result) {
    ArchDaemonWindows::daemonFailed (result);
}

UINT
ArchMiscWindows::getDaemonQuitMessage () {
    return ArchDaemonWindows::getDaemonQuitMessage ();
}

HKEY
ArchMiscWindows::openKey (HKEY key, const TCHAR* keyName) {
    return openKey (key, keyName, false);
}

HKEY
ArchMiscWindows::openKey (HKEY key, const TCHAR* const* keyNames) {
    return openKey (key, keyNames, false);
}

HKEY
ArchMiscWindows::addKey (HKEY key, const TCHAR* keyName) {
    return openKey (key, keyName, true);
}

HKEY
ArchMiscWindows::addKey (HKEY key, const TCHAR* const* keyNames) {
    return openKey (key, keyNames, true);
}

HKEY
ArchMiscWindows::openKey (HKEY key, const TCHAR* keyName, bool create) {
    // ignore if parent is NULL
    if (key == NULL) {
        return NULL;
    }

    // open next key
    HKEY newKey;
    LONG result =
        RegOpenKeyEx (key, keyName, 0, KEY_WRITE | KEY_QUERY_VALUE, &newKey);
    if (result != ERROR_SUCCESS && create) {
        DWORD disp;
        result = RegCreateKeyEx (key,
                                 keyName,
                                 0,
                                 TEXT (""),
                                 0,
                                 KEY_WRITE | KEY_QUERY_VALUE,
                                 NULL,
                                 &newKey,
                                 &disp);
    }
    if (result != ERROR_SUCCESS) {
        RegCloseKey (key);
        return NULL;
    }

    // switch to new key
    RegCloseKey (key);
    return newKey;
}

HKEY
ArchMiscWindows::openKey (HKEY key, const TCHAR* const* keyNames, bool create) {
    for (size_t i = 0; key != NULL && keyNames[i] != NULL; ++i) {
        // open next key
        key = openKey (key, keyNames[i], create);
    }
    return key;
}

void
ArchMiscWindows::closeKey (HKEY key) {
    assert (key != NULL);
    if (key == NULL)
        return;
    RegCloseKey (key);
}

void
ArchMiscWindows::deleteKey (HKEY key, const TCHAR* name) {
    assert (key != NULL);
    assert (name != NULL);
    if (key == NULL || name == NULL)
        return;
    RegDeleteKey (key, name);
}

void
ArchMiscWindows::deleteValue (HKEY key, const TCHAR* name) {
    assert (key != NULL);
    assert (name != NULL);
    if (key == NULL || name == NULL)
        return;
    RegDeleteValue (key, name);
}

bool
ArchMiscWindows::hasValue (HKEY key, const TCHAR* name) {
    DWORD type;
    LONG result = RegQueryValueEx (key, name, 0, &type, NULL, NULL);
    return (result == ERROR_SUCCESS && (type == REG_DWORD || type == REG_SZ));
}

ArchMiscWindows::EValueType
ArchMiscWindows::typeOfValue (HKEY key, const TCHAR* name) {
    DWORD type;
    LONG result = RegQueryValueEx (key, name, 0, &type, NULL, NULL);
    if (result != ERROR_SUCCESS) {
        return kNO_VALUE;
    }
    switch (type) {
        case REG_DWORD:
            return kUINT;

        case REG_SZ:
            return kSTRING;

        case REG_BINARY:
            return kBINARY;

        default:
            return kUNKNOWN;
    }
}

void
ArchMiscWindows::setValue (HKEY key, const TCHAR* name,
                           const std::string& value) {
    assert (key != NULL);
    if (key == NULL) {
        // TODO: throw exception
        return;
    }
    RegSetValueEx (key,
                   name,
                   0,
                   REG_SZ,
                   reinterpret_cast<const BYTE*> (value.c_str ()),
                   (DWORD) value.size () + 1);
}

void
ArchMiscWindows::setValue (HKEY key, const TCHAR* name, DWORD value) {
    assert (key != NULL);
    if (key == NULL) {
        // TODO: throw exception
        return;
    }
    RegSetValueEx (key,
                   name,
                   0,
                   REG_DWORD,
                   reinterpret_cast<CONST BYTE*> (&value),
                   sizeof (DWORD));
}

void
ArchMiscWindows::setValueBinary (HKEY key, const TCHAR* name,
                                 const std::string& value) {
    assert (key != NULL);
    assert (name != NULL);
    if (key == NULL || name == NULL) {
        // TODO: throw exception
        return;
    }
    RegSetValueEx (key,
                   name,
                   0,
                   REG_BINARY,
                   reinterpret_cast<const BYTE*> (value.data ()),
                   (DWORD) value.size ());
}

std::string
ArchMiscWindows::readBinaryOrString (HKEY key, const TCHAR* name, DWORD type) {
    // get the size of the string
    DWORD actualType;
    DWORD size  = 0;
    LONG result = RegQueryValueEx (key, name, 0, &actualType, NULL, &size);
    if (result != ERROR_SUCCESS || actualType != type) {
        return std::string ();
    }

    // if zero size then return empty string
    if (size == 0) {
        return std::string ();
    }

    // allocate space
    char* buffer = new char[size];

    // read it
    result = RegQueryValueEx (
        key, name, 0, &actualType, reinterpret_cast<BYTE*> (buffer), &size);
    if (result != ERROR_SUCCESS || actualType != type) {
        delete[] buffer;
        return std::string ();
    }

    // clean up and return value
    if (type == REG_SZ && buffer[size - 1] == '\0') {
        // don't include terminating nul;  std::string will add one.
        --size;
    }
    std::string value (buffer, size);
    delete[] buffer;
    return value;
}

std::string
ArchMiscWindows::readValueString (HKEY key, const TCHAR* name) {
    return readBinaryOrString (key, name, REG_SZ);
}

std::string
ArchMiscWindows::readValueBinary (HKEY key, const TCHAR* name) {
    return readBinaryOrString (key, name, REG_BINARY);
}

DWORD
ArchMiscWindows::readValueInt (HKEY key, const TCHAR* name) {
    DWORD type;
    DWORD value;
    DWORD size  = sizeof (value);
    LONG result = RegQueryValueEx (
        key, name, 0, &type, reinterpret_cast<BYTE*> (&value), &size);
    if (result != ERROR_SUCCESS || type != REG_DWORD) {
        return 0;
    }
    return value;
}

void
ArchMiscWindows::addDialog (HWND hwnd) {
    s_dialogs->insert (hwnd);
}

void
ArchMiscWindows::removeDialog (HWND hwnd) {
    s_dialogs->erase (hwnd);
}

bool
ArchMiscWindows::processDialog (MSG* msg) {
    for (Dialogs::const_iterator index = s_dialogs->begin ();
         index != s_dialogs->end ();
         ++index) {
        if (IsDialogMessage (*index, msg)) {
            return true;
        }
    }
    return false;
}

void
ArchMiscWindows::addBusyState (DWORD busyModes) {
    s_busyState |= busyModes;
    setThreadExecutionState (s_busyState);
}

void
ArchMiscWindows::removeBusyState (DWORD busyModes) {
    s_busyState &= ~busyModes;
    setThreadExecutionState (s_busyState);
}

void
ArchMiscWindows::setThreadExecutionState (DWORD busyModes) {
    // look up function dynamically so we work on older systems
    if (s_stes == NULL) {
        HINSTANCE kernel = LoadLibrary ("kernel32.dll");
        if (kernel != NULL) {
            s_stes = reinterpret_cast<STES_t> (
                GetProcAddress (kernel, "SetThreadExecutionState"));
        }
        if (s_stes == NULL) {
            s_stes = &ArchMiscWindows::dummySetThreadExecutionState;
        }
    }

    // convert to STES form
    EXECUTION_STATE state = 0;
    if ((busyModes & kSYSTEM) != 0) {
        state |= ES_SYSTEM_REQUIRED;
    }
    if ((busyModes & kDISPLAY) != 0) {
        state |= ES_DISPLAY_REQUIRED;
    }
    if (state != 0) {
        state |= ES_CONTINUOUS;
    }

    // do it
    s_stes (state);
}

DWORD
ArchMiscWindows::dummySetThreadExecutionState (DWORD) {
    // do nothing
    return 0;
}

void
ArchMiscWindows::wakeupDisplay () {
    // We can't use ::setThreadExecutionState here because it sets
    // ES_CONTINUOUS, which we don't want.

    if (s_stes == NULL) {
        HINSTANCE kernel = LoadLibrary ("kernel32.dll");
        if (kernel != NULL) {
            s_stes = reinterpret_cast<STES_t> (
                GetProcAddress (kernel, "SetThreadExecutionState"));
        }
        if (s_stes == NULL) {
            s_stes = &ArchMiscWindows::dummySetThreadExecutionState;
        }
    }

    s_stes (ES_DISPLAY_REQUIRED);

    // restore the original execution states
    setThreadExecutionState (s_busyState);
}

bool
ArchMiscWindows::wasLaunchedAsService () {
    String name;
    if (!getParentProcessName (name)) {
        LOG ((CLOG_ERR "cannot determine if process was launched as service"));
        return false;
    }

    return (name == SERVICE_LAUNCHER);
}

bool
ArchMiscWindows::getParentProcessName (String& name) {
    PROCESSENTRY32 parentEntry;
    if (!getParentProcessEntry (parentEntry)) {
        LOG ((CLOG_ERR "could not get entry for parent process"));
        return false;
    }

    name = parentEntry.szExeFile;
    return true;
}

BOOL WINAPI
ArchMiscWindows::getSelfProcessEntry (PROCESSENTRY32& entry) {
    // get entry from current PID
    return getProcessEntry (entry, GetCurrentProcessId ());
}

BOOL WINAPI
ArchMiscWindows::getParentProcessEntry (PROCESSENTRY32& entry) {
    // get the current process, so we can get parent PID
    PROCESSENTRY32 selfEntry;
    if (!getSelfProcessEntry (selfEntry)) {
        return FALSE;
    }

    // get entry from parent PID
    return getProcessEntry (entry, selfEntry.th32ParentProcessID);
}

BOOL WINAPI
ArchMiscWindows::getProcessEntry (PROCESSENTRY32& entry, DWORD processID) {
    // first we need to take a snapshot of the running processes
    HANDLE snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        LOG ((CLOG_ERR "could not get process snapshot (error: %i)",
              GetLastError ()));
        return FALSE;
    }

    entry.dwSize = sizeof (PROCESSENTRY32);

    // get the first process, and if we can't do that then it's
    // unlikely we can go any further
    BOOL gotEntry = Process32First (snapshot, &entry);
    if (!gotEntry) {
        LOG ((CLOG_ERR "could not get first process entry (error: %i)",
              GetLastError ()));
        return FALSE;
    }

    while (gotEntry) {

        if (entry.th32ProcessID == processID) {
            // found current process
            return TRUE;
        }

        // now move on to the next entry (when we reach end, loop will stop)
        gotEntry = Process32Next (snapshot, &entry);
    }

    return FALSE;
}

HINSTANCE
ArchMiscWindows::instanceWin32 () {
    assert (s_instanceWin32 != NULL);
    return s_instanceWin32;
}

void
ArchMiscWindows::setInstanceWin32 (HINSTANCE instance) {
    assert (instance != NULL);
    s_instanceWin32 = instance;
}
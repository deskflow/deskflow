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

#include "arch/win32/ArchFileWindows.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlobj.h>
#include <tchar.h>
#include <string.h>

//
// ArchFileWindows
//

ArchFileWindows::ArchFileWindows () {
    // do nothing
}

ArchFileWindows::~ArchFileWindows () {
    // do nothing
}

const char*
ArchFileWindows::getBasename (const char* pathname) {
    if (pathname == NULL) {
        return NULL;
    }

    // check for last slash
    const char* basename = strrchr (pathname, '/');
    if (basename != NULL) {
        ++basename;
    } else {
        basename = pathname;
    }

    // check for last backslash
    const char* basename2 = strrchr (pathname, '\\');
    if (basename2 != NULL && basename2 > basename) {
        basename = basename2 + 1;
    }

    return basename;
}

std::string
ArchFileWindows::getUserDirectory () {
    // try %HOMEPATH%
    TCHAR dir[MAX_PATH];
    DWORD size   = sizeof (dir) / sizeof (TCHAR);
    DWORD result = GetEnvironmentVariable (_T("HOMEPATH"), dir, size);
    if (result != 0 && result <= size) {
        // sanity check -- if dir doesn't appear to start with a
        // drive letter and isn't a UNC name then don't use it
        // FIXME -- allow UNC names
        if (dir[0] != '\0' &&
            (dir[1] == ':' || ((dir[0] == '\\' || dir[0] == '/') &&
                               (dir[1] == '\\' || dir[1] == '/')))) {
            return dir;
        }
    }

    // get the location of the personal files.  that's as close to
    // a home directory as we're likely to find.
    ITEMIDLIST* idl;
    if (SUCCEEDED (SHGetSpecialFolderLocation (NULL, CSIDL_PERSONAL, &idl))) {
        TCHAR* path = NULL;
        if (SHGetPathFromIDList (idl, dir)) {
            DWORD attr = GetFileAttributes (dir);
            if (attr != 0xffffffff && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
                path = dir;
        }

        IMalloc* shalloc;
        if (SUCCEEDED (SHGetMalloc (&shalloc))) {
            shalloc->Free (idl);
            shalloc->Release ();
        }

        if (path != NULL) {
            return path;
        }
    }

    // use root of C drive as a default
    return "C:";
}

std::string
ArchFileWindows::getSystemDirectory () {
    // get windows directory
    char dir[MAX_PATH];
    if (GetWindowsDirectory (dir, sizeof (dir)) != 0) {
        return dir;
    } else {
        // can't get it.  use C:\ as a default.
        return "C:";
    }
}

std::string
ArchFileWindows::getInstalledDirectory () {
    char fileNameBuffer[MAX_PATH];
    GetModuleFileName (NULL, fileNameBuffer, MAX_PATH);
    std::string fileName (fileNameBuffer);
    size_t lastSlash = fileName.find_last_of ("\\");
    fileName         = fileName.substr (0, lastSlash);

    return fileName;
}

std::string
ArchFileWindows::getLogDirectory () {
    return getInstalledDirectory ();
}

std::string
ArchFileWindows::getPluginDirectory () {
    if (!m_pluginDirectory.empty ()) {
        return m_pluginDirectory;
    }

    std::string dir = getProfileDirectory ();
    dir.append ("\\Plugins");
    return dir;
}

std::string
ArchFileWindows::getProfileDirectory () {
    String dir;
    if (!m_profileDirectory.empty ()) {
        dir = m_profileDirectory;
    } else {
        TCHAR result[MAX_PATH];
        if (SUCCEEDED (
                SHGetFolderPath (NULL, CSIDL_LOCAL_APPDATA, NULL, 0, result))) {
            dir = result;
        } else {
            dir = getUserDirectory ();
        }
    }

    // HACK: append program name, this seems wrong.
    dir.append ("\\Synergy");

    return dir;
}

std::string
ArchFileWindows::concatPath (const std::string& prefix,
                             const std::string& suffix) {
    std::string path;
    path.reserve (prefix.size () + 1 + suffix.size ());
    path += prefix;
    if (path.size () == 0 ||
        (path[path.size () - 1] != '\\' && path[path.size () - 1] != '/')) {
        path += '\\';
    }
    path += suffix;
    return path;
}

void
ArchFileWindows::setProfileDirectory (const String& s) {
    m_profileDirectory = s;
}

void
ArchFileWindows::setPluginDirectory (const String& s) {
    m_pluginDirectory = s;
}

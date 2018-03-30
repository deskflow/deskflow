/*
 * barrier -- mouse and keyboard sharing utility
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
#include "common/DataDirectories.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlobj.h>
#include <tchar.h>
#include <string.h>

//
// ArchFileWindows
//

const char*
ArchFileWindows::getBasename(const char* pathname)
{
    if (pathname == NULL) {
        return NULL;
    }

    // check for last slash
    const char* basename = strrchr(pathname, '/');
    if (basename != NULL) {
        ++basename;
    }
    else {
        basename = pathname;
    }

    // check for last backslash
    const char* basename2 = strrchr(pathname, '\\');
    if (basename2 != NULL && basename2 > basename) {
        basename = basename2 + 1;
    }

    return basename;
}

std::string
ArchFileWindows::getUserDirectory()
{
    return DataDirectories::personal();
}

std::string
ArchFileWindows::getSystemDirectory()
{
    // get windows directory
    char dir[MAX_PATH];
    if (GetWindowsDirectory(dir, sizeof(dir)) != 0) {
        return dir;
    }
    else {
        // can't get it.  use C:\ as a default.
        return "C:";
    }
}

std::string
ArchFileWindows::getProfileDirectory()
{
    return DataDirectories::profile();
}

std::string
ArchFileWindows::concatPath(const std::string& prefix,
                const std::string& suffix)
{
    std::string path;
    path.reserve(prefix.size() + 1 + suffix.size());
    path += prefix;
    if (path.size() == 0 ||
        (path[path.size() - 1] != '\\' &&
        path[path.size() - 1] != '/')) {
        path += '\\';
    }
    path += suffix;
    return path;
}

void
ArchFileWindows::setProfileDirectory(const String& s)
{
    DataDirectories::profile(s);
}

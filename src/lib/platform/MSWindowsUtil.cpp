/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "platform/MSWindowsUtil.h"

#include "base/String.h"

#include <stdio.h>

//
// MSWindowsUtil
//

String
MSWindowsUtil::getString(HINSTANCE instance, DWORD id)
{
    char* msg = NULL;
    int n = LoadString(instance, id, reinterpret_cast<LPSTR>(&msg), 0);

    if (n <= 0) {
        return String();
    }

    return String (msg, n);
}

String
MSWindowsUtil::getErrorString(HINSTANCE hinstance, DWORD error, DWORD id)
{
    char* buffer;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_IGNORE_INSERTS |
                                FORMAT_MESSAGE_FROM_SYSTEM,
                                0,
                                error,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)&buffer,
                                0,
                                NULL) == 0) {
        String errorString = barrier::string::sprintf("%d", error);
        return barrier::string::format(getString(hinstance, id).c_str(),
                            errorString.c_str());
    }
    else {
        String result(buffer);
        LocalFree(buffer);
        return result;
    }
}

/*

This is a quick and dirty iterative CreateDirectory() wrapper that does zero
error checking. A much better cross-platform option exists in C++17 via
std::filesystem. If/when the project is updated to use 17 this function should
absolutley be replaced!

*/

void
MSWindowsUtil::createDirectory(const std::string& path, bool stripLast)
{
    // create parent directories
    for (auto pos = path.find_first_of('\\'); pos != std::string::npos; pos = path.find_first_of('\\', pos + 1))
        CreateDirectory(path.substr(0, pos).c_str(), NULL);
    if (!stripLast)
        // create innermost directory
        CreateDirectory(path.c_str(), NULL);
}

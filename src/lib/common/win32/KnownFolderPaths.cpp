/*
* barrier -- mouse and keyboard sharing utility
* Copyright (C) 2018 Debauchee Open Source Group
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

#include "KnownFolderPaths.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlobj.h>

static std::string wide_to_mb(const wchar_t* source, int length)
{
    int ansiLength = WideCharToMultiByte(CP_ACP, 0, source, length, NULL, 0, NULL, NULL);
    if (ansiLength > 0) {
        std::string ansiString(ansiLength, 0);
        ansiLength = WideCharToMultiByte(CP_ACP, 0, source, length, &ansiString[0], ansiLength, NULL, NULL);
        if (ansiLength > 0) {
            return ansiString;
        }
    }
    return {};
}

static std::string known_folder_path(const KNOWNFOLDERID& id)
{
    std::string path;
    WCHAR* buffer;
    HRESULT result = SHGetKnownFolderPath(id, 0, NULL, &buffer);
    if (result == S_OK) {
        auto length = lstrlenW(buffer);
        path = wide_to_mb(buffer, length);
        CoTaskMemFree(buffer);
    }
    return path;
}

std::string desktopPath()
{
    return known_folder_path(FOLDERID_Desktop);
}

std::string localAppDataPath()
{
    return known_folder_path(FOLDERID_LocalAppData);
}

std::string programDataPath()
{
    return known_folder_path(FOLDERID_ProgramData);
}

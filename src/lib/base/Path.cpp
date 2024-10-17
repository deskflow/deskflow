/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2021 Symless Ltd.
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
#include "Path.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

namespace deskflow {

namespace filesystem {

#ifdef SYSAPI_WIN32

std::wstring path(const String &filePath)
{
  std::wstring result;

  auto length = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), static_cast<int>(filePath.length()), NULL, 0);
  if (length > 0) {
    result.resize(length);
    MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), static_cast<int>(filePath.length()), &result[0], length);
  }

  return result;
}

#else
std::string path(const String &filePath)
{
  return filePath;
}
#endif

} // namespace filesystem

} // namespace deskflow

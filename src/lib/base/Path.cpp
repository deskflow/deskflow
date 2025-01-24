/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Path.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

namespace deskflow {

namespace filesystem {

#ifdef SYSAPI_WIN32

std::wstring path(const std::string &filePath)
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
std::string path(const std::string &filePath)
{
  return filePath;
}
#endif

} // namespace filesystem

} // namespace deskflow

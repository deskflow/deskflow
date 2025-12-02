/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Path.h"

#if SYSAPI_WIN32
#include <QString>
#endif

namespace deskflow::filesystem {

#ifdef SYSAPI_WIN32

std::wstring path(const std::string &filePath)
{
  const auto qstring = QString::fromStdString(filePath);
  return qstring.toStdWString();
}

#else
std::string path(const std::string &filePath)
{
  return filePath;
}
#endif

} // namespace deskflow::filesystem

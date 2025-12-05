/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Filesystem.h"

namespace deskflow {

std::FILE *fopenUtf8Path(const fs::path &path, const std::string &mode)
{
#if SYSAPI_WIN32
  std::wstring wpath = path.native();
  std::wstring wmode(mode.begin(), mode.end());

  return _wfopen(wpath.c_str(), wmode.c_str());
#else
  return std::fopen(path.native().c_str(), mode.c_str());
#endif
}

} // namespace deskflow

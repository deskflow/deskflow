/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "filesystem.h"

#include "common/common.h"

#include <fstream>

namespace deskflow {

namespace {

template <class Stream> void openUtf8PathImpl(Stream &stream, const fs::path &path, std::ios_base::openmode mode)
{
  stream.open(path.native().c_str(), mode);
}

} // namespace

void openUtf8Path(std::ifstream &stream, const fs::path &path, std::ios_base::openmode mode)
{
  openUtf8PathImpl(stream, path, mode);
}

void openUtf8Path(std::ofstream &stream, const fs::path &path, std::ios_base::openmode mode)
{
  openUtf8PathImpl(stream, path, mode);
}

void openUtf8Path(std::fstream &stream, const fs::path &path, std::ios_base::openmode mode)
{
  openUtf8PathImpl(stream, path, mode);
}

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

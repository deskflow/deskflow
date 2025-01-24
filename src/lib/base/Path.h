/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <common/common.h>
#include <string>

namespace deskflow {

namespace filesystem {

#ifdef SYSAPI_WIN32
std::wstring path(const std::string &filePath);
#else
std::string path(const std::string &filePath);
#endif

} // namespace filesystem

} // namespace deskflow

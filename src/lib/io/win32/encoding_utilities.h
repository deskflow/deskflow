/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: remove dead code if not used in PR #7931

#pragma once

#include <string>
#include <vector>

#include <windows.h>

namespace deskflow {

std::string winWcharToUtf8(const WCHAR *utfStr);
std::vector<WCHAR> utf8ToWinChar(const std::string &str);

} // namespace deskflow

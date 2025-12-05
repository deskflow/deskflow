/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdio>
#include <filesystem>

namespace deskflow {

namespace fs = std::filesystem;

std::FILE *fopenUtf8Path(const fs::path &path, const std::string &mode);

} // namespace deskflow

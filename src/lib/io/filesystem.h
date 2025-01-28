/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: remove dead code if not used in PR #7931

#pragma once

#include <cstdio>
#include <filesystem>
#include <ios>
#include <iosfwd>

namespace deskflow {

namespace fs = std::filesystem;

void openUtf8Path(std::ifstream &stream, const fs::path &path, std::ios_base::openmode mode = std::ios_base::in);
void openUtf8Path(std::ofstream &stream, const fs::path &path, std::ios_base::openmode mode = std::ios_base::out);
void openUtf8Path(
    std::fstream &stream, const fs::path &path, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out
);

std::FILE *fopenUtf8Path(const fs::path &path, const std::string &mode);

} // namespace deskflow

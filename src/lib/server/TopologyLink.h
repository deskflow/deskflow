/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/DirectionTypes.h"

#include <string>

namespace deskflow::server {

//! One directed screen adjacency edge injected from fleet topology (mesh v2).
struct TopologyLink
{
  std::string fromScreen;
  std::string toScreen;
  Direction direction = Direction::NoDirection;
};

} // namespace deskflow::server

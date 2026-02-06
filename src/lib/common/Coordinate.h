/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>
#include <cstdint>

struct Coordinate
{
  Q_GADGET;

public:
  int32_t x;
  int32_t y;
};

using ScrollDelta = Coordinate;

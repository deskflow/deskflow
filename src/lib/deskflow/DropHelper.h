/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Devs
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Ported from Barrier's DropHelper (barrier/src/lib/barrier/DropHelper.cpp)
 * to re-enable file drag and drop (DDRG).
 */

#pragma once

#include "deskflow/DragInformation.h"

#include <string>

class DropHelper
{
public:
  static void writeToDir(const std::string &destination, DragFileList &fileList, std::string &data);
};
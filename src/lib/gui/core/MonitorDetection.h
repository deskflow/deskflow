/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "gui/config/Screen.h"
#include <QVector>

namespace deskflow::gui {

/**
 * @brief Detect monitors on the current system
 * @return Vector of MonitorInfo structures representing detected monitors
 */
QVector<MonitorInfo> detectMonitors();

/**
 * @brief Populate a Screen object with detected monitor information
 * @param screen The Screen object to populate
 */
void populateScreenMonitors(Screen &screen);

} // namespace deskflow::gui


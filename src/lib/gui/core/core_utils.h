/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace deskflow::gui::core_utils {

QString getProfileDir();
QString getInstalledDir();
QString getArch();

} // namespace deskflow::gui::core_utils

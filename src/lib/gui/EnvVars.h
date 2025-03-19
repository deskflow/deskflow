/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace deskflow::gui::env_vars {

inline QString versionUrl()
{
  return qEnvironmentVariable("DESKFLOW_VERSION_URL", QStringLiteral("https://api.deskflow.org/version"));
}

} // namespace deskflow::gui::env_vars

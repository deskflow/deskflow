/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/constants.h"

#include <QString>

/**
 * @brief Get the version number string for display.
 *
 * If the version number ends with ".0", the ".0" is removed.
 * If the version number does not end with ".0", the git SHA is appended in parentheses.
 */
inline QString displayVersion()
{
  auto versionString = QString(kVersion);
  if (versionString.endsWith(QStringLiteral(".0"))) {
    versionString.chop(2);
  } else {
    versionString.append(QStringLiteral(" (%1)").arg(kVersionGitSha));
  }
  return versionString;
}

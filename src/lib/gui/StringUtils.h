/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

/**
 * @brief Useful for environment variables that have string boolean values.
 */
inline bool strToTrue(const QString &str)
{
  return str.toLower() == "true" || str == "1";
}

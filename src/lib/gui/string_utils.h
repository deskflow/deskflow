/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

inline QString trimEnd(const QString &str)
{
  QString result = str;
  while (!result.isEmpty() && result.at(result.size() - 1).isSpace()) {
    result.chop(1);
  }
  return result;
}

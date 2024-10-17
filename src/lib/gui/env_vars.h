/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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
#include <QtCore>

namespace deskflow::gui::env_vars {

inline QString versionUrl()
{
  if (QString(DESKFLOW_VERSION_URL).isEmpty()) {
    qFatal("version url is not set");
  }
  return qEnvironmentVariable("DESKFLOW_VERSION_URL", DESKFLOW_VERSION_URL);
}

} // namespace deskflow::gui::env_vars

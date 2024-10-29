/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
 * Copyright (C) 2015 Symless Ltd.
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

#include "gui/core/CoreTool.h"

#if defined(Q_OS_UNIX)
#include "arch/unix/ArchFileUnix.h"
#include "arch/unix/ArchSystemUnix.h"
#elif defined(Q_OS_WIN)
#include "arch/win32/ArchFileWindows.h"
#include "arch/win32/ArchSystemWindows.h"
#endif

#include <QDir>

QString CoreTool::getProfileDir() const
{
#if defined Q_OS_UNIX
  ArchFileUnix sysInfo;
#elif defined Q_OS_WIN
  ArchFileWindows sysInfo;
#endif
  return QDir::cleanPath(QString::fromUtf8(sysInfo.getProfileDirectory()));
}

QString CoreTool::getInstalledDir() const
{
#if defined Q_OS_UNIX
  ArchFileUnix sysInfo;
#elif defined Q_OS_WIN
  ArchFileWindows sysInfo;
#endif
  return QDir::cleanPath(QString::fromUtf8(sysInfo.getInstalledDirectory()));
}

QString CoreTool::getArch() const
{
#if defined Q_OS_UNIX
  ArchSystemUnix sysInfo;
#elif defined Q_OS_WIN
  ArchSystemWindows sysInfo;
#endif
  return QString::fromUtf8(sysInfo.getPlatformName());
}

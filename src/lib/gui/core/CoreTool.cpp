/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2015 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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

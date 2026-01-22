/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/XArchWindows.h"

#include <QString>

QString windowsErrorToQString(DWORD error)
{
  LPWSTR buffer = nullptr; // Using FORMAT_MESSAGE_ALLOCATE_BUFFER
  DWORD size = FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buffer), 0, nullptr
  );

  if (size == 0) {
    return QString("Unknown Windows error: %1").arg(error);
  }

  QString message = QString::fromWCharArray(buffer, int(size));

  // Gotcha: Was allocated by FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER)
  LocalFree(buffer);

  return QString("[%1] %2").arg(error).arg(message.trimmed());
}

std::string windowsErrorToString(DWORD error)
{
  return windowsErrorToQString(error).toStdString();
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardTextConverter.h"

#include "base/Unicode.h"

#include <QString>

//
// MSWindowsClipboardTextConverter
//

UINT MSWindowsClipboardTextConverter::getWin32Format() const
{
  return CF_TEXT;
}

std::string MSWindowsClipboardTextConverter::doFromIClipboard(const std::string &data) const
{
  return QString::fromStdString(data).toLatin1().toStdString() + '\0';
}

std::string MSWindowsClipboardTextConverter::doToIClipboard(const std::string &data) const
{
  auto q = QString::fromStdString(data);
  q.truncate(q.indexOf('\0'));
  return q.toStdString();
}

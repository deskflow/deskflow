/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardTextConverter.h"

#include "base/Unicode.h"

//
// MSWindowsClipboardTextConverter
//

MSWindowsClipboardTextConverter::MSWindowsClipboardTextConverter()
{
  // do nothing
}

MSWindowsClipboardTextConverter::~MSWindowsClipboardTextConverter()
{
  // do nothing
}

UINT MSWindowsClipboardTextConverter::getWin32Format() const
{
  return CF_TEXT;
}

std::string MSWindowsClipboardTextConverter::doFromIClipboard(const std::string &data) const
{
  // convert and add nul terminator
  return Unicode::UTF8ToText(data) += '\0';
}

std::string MSWindowsClipboardTextConverter::doToIClipboard(const std::string &data) const
{
  // convert and truncate at first nul terminator
  std::string dst = Unicode::textToUTF8(data);
  std::string::size_type n = dst.find('\0');
  if (n != std::string::npos) {
    dst.erase(n);
  }
  return dst;
}

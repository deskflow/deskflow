/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardUTF16Converter.h"

#include "base/Unicode.h"

//
// MSWindowsClipboardUTF16Converter
//

MSWindowsClipboardUTF16Converter::MSWindowsClipboardUTF16Converter()
{
  // do nothing
}

MSWindowsClipboardUTF16Converter::~MSWindowsClipboardUTF16Converter()
{
  // do nothing
}

UINT MSWindowsClipboardUTF16Converter::getWin32Format() const
{
  return CF_UNICODETEXT;
}

std::string MSWindowsClipboardUTF16Converter::doFromIClipboard(const std::string &data) const
{
  // convert and add nul terminator
  return Unicode::UTF8ToUTF16(data).append(sizeof(wchar_t), 0);
}

std::string MSWindowsClipboardUTF16Converter::doToIClipboard(const std::string &data) const
{
  // convert and strip nul terminator
  std::string dst = Unicode::UTF16ToUTF8(data);
  std::string::size_type n = dst.find('\0');
  if (n != std::string::npos) {
    dst.erase(n);
  }
  return dst;
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardAnyTextConverter.h"

#include <QByteArray>

//
// MSWindowsClipboardAnyTextConverter
//

IClipboard::Format MSWindowsClipboardAnyTextConverter::getFormat() const
{
  return IClipboard::Format::Text;
}

HANDLE
MSWindowsClipboardAnyTextConverter::fromIClipboard(const std::string &data) const
{
  // convert linefeeds and then convert to desired encoding
  std::string text = doFromIClipboard(convertLinefeedToWin32(data));
  uint32_t size = (uint32_t)text.size();

  // copy to memory handle
  HGLOBAL gData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size);
  if (gData != nullptr) {
    // get a pointer to the allocated memory
    char *dst = (char *)GlobalLock(gData);
    if (dst != nullptr) {
      memcpy(dst, text.data(), size);
      GlobalUnlock(gData);
    } else {
      GlobalFree(gData);
      gData = nullptr;
    }
  }

  return gData;
}

std::string MSWindowsClipboardAnyTextConverter::toIClipboard(HANDLE data) const
{
  // get datator
  const char *src = (const char *)GlobalLock(data);
  uint32_t srcSize = (uint32_t)GlobalSize(data);
  if (src == nullptr || srcSize <= 1) {
    return std::string();
  }

  // convert text
  std::string text = doToIClipboard(std::string(src, srcSize));

  // release handle
  GlobalUnlock(data);

  // convert newlines
  return convertLinefeedToUnix(text);
}

std::string MSWindowsClipboardAnyTextConverter::convertLinefeedToWin32(const std::string &src) const
{
  return QByteArray::fromStdString(src).replace('\n', "\r\n").toStdString();
}

std::string MSWindowsClipboardAnyTextConverter::convertLinefeedToUnix(const std::string &src) const
{
  return QByteArray::fromStdString(src).replace("\r\n", "\n").toStdString();
}

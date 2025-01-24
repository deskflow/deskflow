/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardAnyTextConverter.h"

//
// MSWindowsClipboardAnyTextConverter
//

MSWindowsClipboardAnyTextConverter::MSWindowsClipboardAnyTextConverter()
{
  // do nothing
}

MSWindowsClipboardAnyTextConverter::~MSWindowsClipboardAnyTextConverter()
{
  // do nothing
}

IClipboard::EFormat MSWindowsClipboardAnyTextConverter::getFormat() const
{
  return IClipboard::kText;
}

HANDLE
MSWindowsClipboardAnyTextConverter::fromIClipboard(const std::string &data) const
{
  // convert linefeeds and then convert to desired encoding
  std::string text = doFromIClipboard(convertLinefeedToWin32(data));
  uint32_t size = (uint32_t)text.size();

  // copy to memory handle
  HGLOBAL gData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size);
  if (gData != NULL) {
    // get a pointer to the allocated memory
    char *dst = (char *)GlobalLock(gData);
    if (dst != NULL) {
      memcpy(dst, text.data(), size);
      GlobalUnlock(gData);
    } else {
      GlobalFree(gData);
      gData = NULL;
    }
  }

  return gData;
}

std::string MSWindowsClipboardAnyTextConverter::toIClipboard(HANDLE data) const
{
  // get datator
  const char *src = (const char *)GlobalLock(data);
  uint32_t srcSize = (uint32_t)GlobalSize(data);
  if (src == NULL || srcSize <= 1) {
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
  // note -- we assume src is a valid UTF-8 string

  // count newlines in string
  uint32_t numNewlines = 0;
  uint32_t n = (uint32_t)src.size();
  for (const char *scan = src.c_str(); n > 0; ++scan, --n) {
    if (*scan == '\n') {
      ++numNewlines;
    }
  }
  if (numNewlines == 0) {
    return src;
  }

  // allocate new string
  std::string dst;
  dst.reserve(src.size() + numNewlines);

  // copy string, converting newlines
  n = (uint32_t)src.size();
  for (const char *scan = src.c_str(); n > 0; ++scan, --n) {
    if (scan[0] == '\n') {
      dst += '\r';
    }
    dst += scan[0];
  }

  return dst;
}

std::string MSWindowsClipboardAnyTextConverter::convertLinefeedToUnix(const std::string &src) const
{
  // count newlines in string
  uint32_t numNewlines = 0;
  uint32_t n = (uint32_t)src.size();
  for (const char *scan = src.c_str(); n > 0; ++scan, --n) {
    if (scan[0] == '\r' && scan[1] == '\n') {
      ++numNewlines;
    }
  }
  if (numNewlines == 0) {
    return src;
  }

  // allocate new string
  std::string dst;
  dst.reserve(src.size());

  // copy string, converting newlines
  n = (uint32_t)src.size();
  for (const char *scan = src.c_str(); n > 0; ++scan, --n) {
    if (scan[0] != '\r' || scan[1] != '\n') {
      dst += scan[0];
    }
  }

  return dst;
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardRawConverter.h"

#include "base/Log.h"

#include <cstring>

MSWindowsClipboardRawConverter::MSWindowsClipboardRawConverter(const wchar_t *formatName, IClipboard::Format format)
    : m_win32Format(RegisterClipboardFormatW(formatName)),
      m_format(format)
{
}

MSWindowsClipboardRawConverter::MSWindowsClipboardRawConverter(UINT win32Format, IClipboard::Format format)
    : m_win32Format(win32Format),
      m_format(format)
{
}

IClipboard::Format MSWindowsClipboardRawConverter::getFormat() const
{
  return m_format;
}

UINT MSWindowsClipboardRawConverter::getWin32Format() const
{
  return m_win32Format;
}

HANDLE MSWindowsClipboardRawConverter::fromIClipboard(const std::string &data) const
{
  HGLOBAL gData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, data.size());
  if (gData != nullptr) {
    auto *dst = static_cast<char *>(GlobalLock(gData));
    if (dst != nullptr) {
      std::memcpy(dst, data.data(), data.size());
      GlobalUnlock(gData);
    } else {
      GlobalFree(gData);
      gData = nullptr;
    }
  }

  return gData;
}

std::string MSWindowsClipboardRawConverter::toIClipboard(HANDLE data) const
{
  const auto *src = static_cast<const char *>(GlobalLock(data));
  if (src == nullptr) {
    LOG_WARN("failed to lock clipboard data, format: %d", static_cast<int>(m_format));
    return {};
  }

  std::string contents(src, GlobalSize(data));
  GlobalUnlock(data);
  return contents;
}

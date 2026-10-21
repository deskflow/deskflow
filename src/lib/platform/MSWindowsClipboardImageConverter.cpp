/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardImageConverter.h"

#include "base/Log.h"
#include "platform/ClipboardImage.h"

#include <cstring>

MSWindowsClipboardImageConverter::MSWindowsClipboardImageConverter(const wchar_t *formatName, const char *imageFormat)
    : m_format(RegisterClipboardFormatW(formatName)),
      m_imageFormat(imageFormat)
{
}

IClipboard::Format MSWindowsClipboardImageConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

UINT MSWindowsClipboardImageConverter::getWin32Format() const
{
  return m_format;
}

HANDLE MSWindowsClipboardImageConverter::fromIClipboard(const std::string &data) const
{
  const auto encoded = deskflow::ClipboardImage::dibToImage(QByteArray::fromStdString(data), m_imageFormat);
  if (encoded.isEmpty()) {
    return nullptr;
  }

  HGLOBAL gData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, static_cast<SIZE_T>(encoded.size()));
  if (gData != nullptr) {
    auto *dst = static_cast<char *>(GlobalLock(gData));
    if (dst != nullptr) {
      std::memcpy(dst, encoded.constData(), static_cast<size_t>(encoded.size()));
      GlobalUnlock(gData);
    } else {
      GlobalFree(gData);
      gData = nullptr;
    }
  }

  return gData;
}

std::string MSWindowsClipboardImageConverter::toIClipboard(HANDLE data) const
{
  const auto *src = static_cast<const char *>(GlobalLock(data));
  if (src == nullptr) {
    LOG_WARN("failed to lock clipboard image data, format: %s", m_imageFormat);
    return {};
  }

  const QByteArray encoded(src, static_cast<qsizetype>(GlobalSize(data)));
  GlobalUnlock(data);

  return deskflow::ClipboardImage::imageToDib(encoded, m_imageFormat).toStdString();
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016, 2026 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboard.h"

#include "base/Log.h"
#include "platform/ClipboardImage.h"
#include "platform/MSWindowsClipboardBitmapConverter.h"
#include "platform/MSWindowsClipboardFacade.h"
#include "platform/MSWindowsClipboardHTMLConverter.h"
#include "platform/MSWindowsClipboardImageConverter.h"
#include "platform/MSWindowsClipboardRawConverter.h"
#include "platform/MSWindowsClipboardUTF16Converter.h"

#include <algorithm>
#include <functional>
#include <iterator>

#include <QByteArrayList>
#include <QString>

//
// MSWindowsClipboard
//

UINT MSWindowsClipboard::s_ownershipFormat = 0;

MSWindowsClipboard::MSWindowsClipboard(HWND window)
    : m_window(window),
      m_time(0),
      m_facade(new MSWindowsClipboardFacade()),
      m_deleteFacade(true)
{
  // add converters, most desired first
  m_converters.push_back(new MSWindowsClipboardUTF16Converter);

  // apps look for image files under the standard names, and qt apps under the mime type
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"PNG", IClipboard::Format::PNG));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"image/png", IClipboard::Format::PNG));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"JFIF", IClipboard::Format::JPEG));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"image/jpeg", IClipboard::Format::JPEG));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"image/webp", IClipboard::Format::WebP));
  m_converters.push_back(new MSWindowsClipboardRawConverter(CF_TIFF, IClipboard::Format::TIFF));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"image/tiff", IClipboard::Format::TIFF));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"GIF", IClipboard::Format::GIF));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"image/gif", IClipboard::Format::GIF));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"image/svg+xml", IClipboard::Format::SVG));

  // raw pixels: the bitmap is written for apps that only paste bitmaps, and all of these are read to send as png
  m_converters.push_back(new MSWindowsClipboardBitmapConverter);
  m_converters.push_back(new MSWindowsClipboardImageConverter(L"image/bmp", "BMP"));

  m_converters.push_back(new MSWindowsClipboardHTMLConverter);
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"Rich Text Format", IClipboard::Format::RTF));
  m_converters.push_back(new MSWindowsClipboardRawConverter(L"text/rtf", IClipboard::Format::RTF));
}

MSWindowsClipboard::~MSWindowsClipboard()
{
  clearConverters();

  // dependency injection causes confusion over ownership, so we need
  // logic to decide whether or not we delete the facade. there must
  // be a more elegant way of doing this.
  if (m_deleteFacade)
    delete m_facade;
}

void MSWindowsClipboard::setFacade(IMSWindowsClipboardFacade &facade)
{
  delete m_facade;
  m_facade = &facade;
  m_deleteFacade = false;
}

bool MSWindowsClipboard::emptyUnowned()
{
  LOG_DEBUG("empty clipboard");

  // empty the clipboard (and take ownership)
  if (!EmptyClipboard()) {
    // unable to cause this in integ tests, but this error has never
    // actually been reported by users.
    LOG_WARN("failed to grab clipboard");
    return false;
  }

  return true;
}

bool MSWindowsClipboard::empty()
{
  if (!emptyUnowned()) {
    return false;
  }

  // mark clipboard as being owned by deskflow
  HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, 1);
  if (nullptr == SetClipboardData(getOwnershipFormat(), data)) {
    LOG_WARN("failed to set clipboard data");
    GlobalFree(data);
    return false;
  }

  return true;
}

void MSWindowsClipboard::add(Format format, const std::string &data)
{
  // exit early if there is no data to prevent spurious "failed to convert clipboard data" errors
  if (data.empty()) {
    LOG_DEBUG("not adding 0 bytes to clipboard format: %d", format);
    return;
  }

  // apps look for a file under different names, so it goes under all of them
  const bool writeEveryConverter = isFile(format);
  bool isSucceeded = false;
  for (auto *converter : m_converters) {
    if (converter->getFormat() == format && (!isSucceeded || writeEveryConverter)) {
      HANDLE win32Data = converter->fromIClipboard(data);
      if (win32Data != nullptr) {
        const auto size = formatSize(data.size());
        LOG_DEBUG("adding to clipboard, format: %d, size: %s", format, size.constData());
        m_facade->write(win32Data, converter->getWin32Format());
        isSucceeded = true;
      } else {
        LOG_DEBUG("failed to convert clipboard data to platform format");
      }
    }
  }

  if (!isSucceeded) {
    LOG_DEBUG("missed clipboard data convert for format: %d", format);
  }

  // paint and office only paste bitmaps
  const auto *imageFormat = deskflow::ClipboardImage::qtFormat(format);
  if (isSucceeded && imageFormat != nullptr && !IsClipboardFormatAvailable(CF_DIB)) {
    const auto dib = deskflow::ClipboardImage::imageToDib(QByteArray::fromStdString(data), imageFormat);
    if (!dib.isEmpty())
      add(Format::Bitmap, dib.toStdString());
  }
}

bool MSWindowsClipboard::open(Time time) const
{
  LOG_DEBUG("open clipboard");

  // The clipboard is a global mutex on Windows. We aren't always going to
  // get the lock on the first try, so try a few times before giving up.
  // Based on Chromium's ScopedClipboard::Acquire() retry loop.
  static const int kMaxRetries = 5;
  static const int kRetryDelayMs = 5;

  for (int i = 0; i < kMaxRetries; ++i) {
    if (OpenClipboard(m_window)) {
      std::scoped_lock lock{m_mutex};
      m_time = time;
      return true;
    }

    if (i < kMaxRetries - 1) {
      LOG_DEBUG("failed to open clipboard (attempt %d/%d, error=%d), retrying", i + 1, kMaxRetries, GetLastError());
      Sleep(kRetryDelayMs);
    }
  }

  LOG_WARN("failed to open clipboard after %d attempts: %d", kMaxRetries, GetLastError());
  return false;
}

void MSWindowsClipboard::close() const
{
  LOG_DEBUG("close clipboard");
  CloseClipboard();
}

IClipboard::Time MSWindowsClipboard::getTime() const
{
  std::scoped_lock lock{m_mutex};
  return m_time;
}

bool MSWindowsClipboard::has(Format format) const
{
  return sourceToSend(format, std::bind_front(&MSWindowsClipboard::isAvailable, this)).has_value();
}

std::string MSWindowsClipboard::get(Format format) const
{
  const auto source = sourceToSend(format, std::bind_front(&MSWindowsClipboard::isAvailable, this));
  std::string data;
  if (source == Format::Bitmap) {
    data = deskflow::ClipboardImage::dibToImage(QByteArray::fromStdString(read(Format::Bitmap)), "PNG").toStdString();
  } else if (source) {
    data = read(*source);
  } else {
    LOG_WARN("clipboard has nothing to send for format: %d", format);
  }
  return data;
}

bool MSWindowsClipboard::isAvailable(Format format) const
{
  for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    IMSWindowsClipboardConverter *converter = *index;
    if (converter->getFormat() == format) {
      if (IsClipboardFormatAvailable(converter->getWin32Format())) {
        return true;
      }
    }
  }
  return false;
}

std::string MSWindowsClipboard::read(Format format) const
{
  // an image file can be under several names, so use the first one on the clipboard
  IMSWindowsClipboardConverter *converter = nullptr;
  for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {

    converter = *index;
    if (converter->getFormat() == format && IsClipboardFormatAvailable(converter->getWin32Format())) {
      break;
    }
    converter = nullptr;
  }

  // if no converter then we don't recognize any formats
  if (converter == nullptr) {
    LOG_WARN("no converter for format %d", format);
    return std::string();
  }

  // get a handle to the clipboard data
  HANDLE win32Data = GetClipboardData(converter->getWin32Format());
  if (win32Data == nullptr) {
    // nb: can't cause this using integ tests; this is only caused when
    // the selected converter returns an invalid format -- which you
    // cannot cause using public functions.
    return std::string();
  }

  // convert
  return converter->toIClipboard(win32Data);
}

void MSWindowsClipboard::logUnreadableFormats() const
{
  const bool readable = std::ranges::any_of(m_converters, [](const IMSWindowsClipboardConverter *converter) {
    return IsClipboardFormatAvailable(converter->getWin32Format()) != 0;
  });
  if (readable || CountClipboardFormats() == 0) {
    return;
  }

  QByteArrayList formats;
  if (open(0)) {
    UINT format = 0;
    while ((format = EnumClipboardFormats(format)) != 0) {
      wchar_t name[256];
      const int length = GetClipboardFormatNameW(format, name, static_cast<int>(std::size(name)));
      formats.append(length > 0 ? QString::fromWCharArray(name, length).toUtf8() : QByteArray::number(format));
    }
    close();
  }
  LOG_DEBUG("clipboard has no format we can read, formats: %s", formats.join(", ").constData());
}

void MSWindowsClipboard::clearConverters()
{
  for (ConverterList::iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    delete *index;
  }
  m_converters.clear();
}

bool MSWindowsClipboard::isOwnedByDeskflow()
{
  // create ownership format if we haven't yet
  if (s_ownershipFormat == 0) {
    s_ownershipFormat = RegisterClipboardFormat(TEXT("Deskflow Ownership"));
  }
  return (IsClipboardFormatAvailable(getOwnershipFormat()) != 0);
}

UINT MSWindowsClipboard::getOwnershipFormat()
{
  // create ownership format if we haven't yet
  if (s_ownershipFormat == 0) {
    s_ownershipFormat = RegisterClipboardFormat(TEXT("Deskflow Ownership"));
  }

  // return the format
  return s_ownershipFormat;
}

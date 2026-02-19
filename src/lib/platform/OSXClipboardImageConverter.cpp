/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardImageConverter.h"

#include "platform/ClipboardImageConverter.h"

#include <CoreFoundation/CoreFoundation.h>
#include <ImageIO/ImageIO.h>

namespace {

std::string decodeImageToBitmapViaImageIO(const std::string &data)
{
  if (data.empty()) {
    return {};
  }

  auto inData = CFDataCreate(
      kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data.data()), static_cast<CFIndex>(data.size())
  );
  if (inData == nullptr) {
    return {};
  }

  auto source = CGImageSourceCreateWithData(inData, nullptr);
  CFRelease(inData);
  if (source == nullptr) {
    return {};
  }

  auto image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
  CFRelease(source);
  if (image == nullptr) {
    return {};
  }

  auto outData = CFDataCreateMutable(kCFAllocatorDefault, 0);
  if (outData == nullptr) {
    CGImageRelease(image);
    return {};
  }

  auto destination = CGImageDestinationCreateWithData(outData, CFSTR("public.png"), 1, nullptr);
  if (destination == nullptr) {
    CFRelease(outData);
    CGImageRelease(image);
    return {};
  }

  CGImageDestinationAddImage(destination, image, nullptr);
  const auto finalized = CGImageDestinationFinalize(destination);
  CFRelease(destination);
  CGImageRelease(image);

  if (!finalized) {
    CFRelease(outData);
    return {};
  }

  std::string png(
      reinterpret_cast<const char *>(CFDataGetBytePtr(outData)), static_cast<size_t>(CFDataGetLength(outData))
  );
  CFRelease(outData);

  return deskflow::platform::clipboard::decodeImageToBitmap(png, "PNG");
}

} // namespace

OSXClipboardImageConverter::OSXClipboardImageConverter(CFStringRef osxFormat, const char *imageFormatHint)
    : m_osxFormat(osxFormat),
      m_imageFormatHint(imageFormatHint)
{
}

IClipboard::Format OSXClipboardImageConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

CFStringRef OSXClipboardImageConverter::getOSXFormat() const
{
  return m_osxFormat;
}

std::string OSXClipboardImageConverter::fromIClipboard(const std::string &data) const
{
  return deskflow::platform::clipboard::encodeBitmapToImage(data, m_imageFormatHint);
}

std::string OSXClipboardImageConverter::toIClipboard(const std::string &data) const
{
  auto converted = deskflow::platform::clipboard::decodeImageToBitmap(data, m_imageFormatHint);
  if (!converted.empty() || data.empty()) {
    return converted;
  }

  return decodeImageToBitmapViaImageIO(data);
}

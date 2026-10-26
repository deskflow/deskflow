/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardImageConverter.h"

#include "base/Log.h"

#include <ImageIO/ImageIO.h>

OSXClipboardImageConverter::OSXClipboardImageConverter(const char *uti, const char *aliasUti)
    : m_utiName(uti),
      m_uti(CFStringCreateWithCString(kCFAllocatorDefault, uti, kCFStringEncodingUTF8)),
      m_aliasUti(aliasUti ? CFStringCreateWithCString(kCFAllocatorDefault, aliasUti, kCFStringEncodingUTF8) : nullptr)
{
}

OSXClipboardImageConverter::~OSXClipboardImageConverter()
{
  CFRelease(m_uti);
  if (m_aliasUti != nullptr) {
    CFRelease(m_aliasUti);
  }
}

IClipboard::Format OSXClipboardImageConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

CFStringRef OSXClipboardImageConverter::getOSXFormat() const
{
  return m_uti;
}

CFStringRef OSXClipboardImageConverter::getAliasOSXFormat() const
{
  return m_aliasUti;
}

std::string OSXClipboardImageConverter::fromIClipboard(const std::string &dib) const
{
  const auto bmp = m_bmpConverter.fromIClipboard(dib);
  if (bmp.empty()) {
    LOG_WARN("failed to convert clipboard bitmap to bmp");
    return {};
  }

  auto encoded = convertImage(bmp, m_uti);
  if (encoded.empty()) {
    LOG_DEBUG("failed to encode clipboard image, format: %s", m_utiName);
  }
  return encoded;
}

std::string OSXClipboardImageConverter::toIClipboard(const std::string &encoded) const
{
  const auto bmp = convertImage(encoded, m_bmpConverter.getOSXFormat());
  if (bmp.empty()) {
    LOG_WARN("failed to decode clipboard image, format: %s", m_utiName);
    return {};
  }

  return m_bmpConverter.toIClipboard(bmp);
}

std::string OSXClipboardImageConverter::convertImage(const std::string &image, CFStringRef toType)
{
  CFDataRef sourceData = CFDataCreate(
      kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(image.data()), static_cast<CFIndex>(image.size())
  );
  if (sourceData == nullptr) {
    return {};
  }

  CGImageSourceRef source = CGImageSourceCreateWithData(sourceData, nullptr);
  CFRelease(sourceData);
  if (source == nullptr) {
    return {};
  }

  CGImageRef decoded = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
  CFRelease(source);
  if (decoded == nullptr) {
    return {};
  }

  std::string converted;
  CFMutableDataRef convertedData = CFDataCreateMutable(kCFAllocatorDefault, 0);
  CGImageDestinationRef destination =
      convertedData != nullptr ? CGImageDestinationCreateWithData(convertedData, toType, 1, nullptr) : nullptr;
  if (destination != nullptr) {
    CGImageDestinationAddImage(destination, decoded, nullptr);
    if (CGImageDestinationFinalize(destination)) {
      converted.assign(
          reinterpret_cast<const char *>(CFDataGetBytePtr(convertedData)),
          static_cast<size_t>(CFDataGetLength(convertedData))
      );
    }
    CFRelease(destination);
  }
  if (convertedData != nullptr) {
    CFRelease(convertedData);
  }
  CGImageRelease(decoded);

  return converted;
}

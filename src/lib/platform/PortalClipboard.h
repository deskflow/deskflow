/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"

#include <QByteArray>

#include <libportal/portal.h>

namespace deskflow {

class EiClipboard;

class PortalClipboard
{
public:
  struct SupportedMime
  {
    const char *mime;
    IClipboard::Format format;
    const char *imageFormat;
    bool offered;
  };

  // Listed in preference order: richer formats first. Bitmaps are offered as png but read from any image type.
  static constexpr SupportedMime kSupportedMimes[] = {
      {"image/gif", IClipboard::Format::GIF, nullptr, true},
      {"image/svg+xml", IClipboard::Format::SVG, nullptr, true},
      {"image/png", IClipboard::Format::Bitmap, "PNG", true},
      {"image/jpeg", IClipboard::Format::Bitmap, "JPEG", false},
      {"image/bmp", IClipboard::Format::Bitmap, "BMP", false},
      {"image/tiff", IClipboard::Format::Bitmap, "TIFF", false},
      {"image/webp", IClipboard::Format::Bitmap, "WEBP", false},
      {"text/plain;charset=utf-8", IClipboard::Format::Text, nullptr, true},
      {"text/plain", IClipboard::Format::Text, nullptr, true},
  };

  static constexpr int kReadTimeoutMs = 200;
  static constexpr int kWriteTimeoutMs = 200;
  static constexpr qint64 kChunkBytes = 64 * 1024;

  static QByteArray formatMimeTypes(const char *const *mimeTypes);
  static const SupportedMime *findSupportedMime(const char *mime);
  static const SupportedMime *pickSupportedMime(const char *const *available);
  static QByteArray encodeFormat(const SupportedMime &entry, const QByteArray &data);
  static QByteArray decodeFormat(const SupportedMime &entry, const QByteArray &bytes);
  static QByteArray readSelectionBytes(XdpSession *session, const char *mime, qint64 maxBytes);

  /// Advertise the cache's formats to the portal selection.
  static void claimOwnership(EiClipboard *cache, XdpSession *session);

  /// Respond to a selection-transfer signal by writing the cache's bytes for
  /// \p mime to the portal-provided fd. Always calls write_done.
  static void serveSelectionTransfer(EiClipboard *cache, XdpSession *session, const char *mime, uint32_t serial);

  /// Read every supported format offered by the portal into the cache.
  /// Returns true if any data was deposited.
  static bool
  readSelectionIntoCache(EiClipboard *cache, XdpSession *session, const char *const *mimeTypes, qint64 maxBytes);
};

} // namespace deskflow

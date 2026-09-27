/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"

#include <optional>

#include <QByteArray>

#include <libportal/portal.h>

class QIODevice;

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

  // Listed in preference order. Other image types are only read, as raw pixels, and sent as a png.
  static constexpr SupportedMime kSupportedMimes[] = {
      {"image/gif", IClipboard::Format::GIF, nullptr, true},
      {"image/svg+xml", IClipboard::Format::SVG, nullptr, true},
      {"image/png", IClipboard::Format::PNG, nullptr, true},
      {"image/jpeg", IClipboard::Format::JPEG, nullptr, true},
      {"image/bmp", IClipboard::Format::Bitmap, "BMP", false},
      {"image/tiff", IClipboard::Format::Bitmap, "TIFF", false},
      {"image/webp", IClipboard::Format::Bitmap, "WEBP", false},
      {"text/html", IClipboard::Format::HTML, nullptr, true},
      {"text/plain;charset=utf-8", IClipboard::Format::Text, nullptr, true},
      {"text/plain", IClipboard::Format::Text, nullptr, true},
  };

  // apps that only take png get one converted from a jpeg or raw pixels
  static constexpr IClipboard::Format kPngSources[] = {
      IClipboard::Format::PNG, IClipboard::Format::JPEG, IClipboard::Format::Bitmap
  };

  static constexpr int kWriteTimeoutMs = 200;
  static constexpr qint64 kChunkBytes = 64 * 1024;

  // apps may only encode an image when it's pasted, which takes seconds for a large one
  static constexpr int kReadTimeoutMs = 5000;

  static QByteArray formatMimeTypes(const char *const *mimeTypes);
  static const SupportedMime *findSupportedMime(const char *mime);
  static const SupportedMime *pickSupportedMime(const char *const *available);
  static std::optional<IClipboard::Format> heldFormat(EiClipboard *cache, IClipboard::Format format);
  static bool
  writeFormat(IClipboard::Format format, IClipboard::Format held, const QByteArray &data, QIODevice *device);
  static QByteArray decodeFormat(const SupportedMime &entry, const QByteArray &bytes);
  static std::optional<QByteArray> readSelectionBytes(XdpSession *session, const char *mime, qint64 maxBytes);
  static std::optional<QByteArray>
  readFormat(XdpSession *session, const char *const *mimeTypes, IClipboard::Format format, qint64 maxBytes);

  /// Advertise the cache's formats to the portal selection.
  static void claimOwnership(EiClipboard *cache, XdpSession *session);

  /// Respond to a selection-transfer signal by writing the cache's bytes for
  /// \p mime to the portal-provided fd. Always calls write_done.
  static void serveSelectionTransfer(EiClipboard *cache, XdpSession *session, const char *mime, uint32_t serial);

  /// Read every supported format offered by the portal into the cache.
  /// Returns true if any data was deposited.
  static bool
  readSelectionIntoCache(EiClipboard *cache, XdpSession *session, const char *const *mimeTypes, qint64 maxBytes);

private:
  class SelectionPipe;
};

} // namespace deskflow

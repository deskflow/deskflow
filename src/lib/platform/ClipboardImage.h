/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QByteArray>

class QIODevice;

namespace deskflow {

class ClipboardImage
{
public:
  static QByteArray dibToImage(const QByteArray &dib, const char *format);
  static bool writeDibAsImage(const QByteArray &dib, const char *format, QIODevice *device);
  static QByteArray imageToDib(const QByteArray &encoded, const char *format);

private:
  static QByteArray dibToBmp(const QByteArray &dib);
  static QByteArray bmpToDib(const QByteArray &bmp);

  static constexpr int kBmpSignatureSize = 2;
  static constexpr quint32 kBmpFileHeaderSize = 14;
  static constexpr quint32 kMinDibHeaderSize = 12;

  // zlib level 1: much faster than qt's default for large images, at a similar size
  static constexpr int kPngCompression = 20;
};

} // namespace deskflow

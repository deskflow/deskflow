/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/ClipboardImage.h"

#include "base/Log.h"

#include <cstring>

#include <QBuffer>
#include <QDataStream>
#include <QImage>
#include <QtEndian>

namespace deskflow {

QByteArray ClipboardImage::dibToImage(const QByteArray &dib, const char *format)
{
  const auto bmpFile = dibToBmp(dib);
  if (bmpFile.isEmpty()) {
    LOG_WARN("clipboard bitmap data is malformed");
    return {};
  }

  QImage image;
  if (!image.loadFromData(bmpFile, "BMP")) {
    LOG_WARN("failed to decode clipboard bitmap");
    return {};
  }

  QByteArray encoded;
  QBuffer buf(&encoded);
  buf.open(QIODevice::WriteOnly);
  if (!image.save(&buf, format)) {
    LOG_WARN("failed to encode clipboard image, format: %s", format);
    return {};
  }

  return encoded;
}

QByteArray ClipboardImage::imageToDib(const QByteArray &encoded, const char *format)
{
  QImage image;
  if (!image.loadFromData(encoded, format)) {
    LOG_WARN("failed to decode clipboard image, format: %s", format);
    return {};
  }

  QByteArray bmp;
  QBuffer buf(&bmp);
  buf.open(QIODevice::WriteOnly);
  if (!image.save(&buf, "BMP")) {
    LOG_WARN("failed to encode clipboard image as bmp");
    return {};
  }

  return bmpToDib(bmp);
}

QByteArray ClipboardImage::dibToBmp(const QByteArray &dib)
{
  if (dib.size() < static_cast<qint64>(sizeof(quint32)))
    return {};

  quint32 headerSize;
  std::memcpy(&headerSize, dib.constData(), sizeof(headerSize));
  headerSize = qFromLittleEndian(headerSize);
  if (headerSize < kMinDibHeaderSize || headerSize > static_cast<quint32>(dib.size()))
    return {};

  const auto fileSize = static_cast<quint32>(kBmpFileHeaderSize + dib.size());
  const quint32 pixelOffset = kBmpFileHeaderSize + headerSize;

  QByteArray bmp;
  QDataStream ds(&bmp, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::LittleEndian);
  ds.writeRawData("BM", kBmpSignatureSize);
  ds << fileSize;
  ds << quint32(0);
  ds << pixelOffset;
  ds.writeRawData(dib.constData(), static_cast<int>(dib.size()));
  return bmp;
}

QByteArray ClipboardImage::bmpToDib(const QByteArray &bmp)
{
  if (bmp.size() < kBmpFileHeaderSize)
    return {};

  return bmp.mid(kBmpFileHeaderSize);
}

} // namespace deskflow

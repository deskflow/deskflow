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
#include <QImageReader>
#include <QImageWriter>
#include <QtEndian>

namespace deskflow {

QByteArray ClipboardImage::dibToImage(const QByteArray &dib, const char *format)
{
  QByteArray encoded;
  QBuffer buf(&encoded);
  buf.open(QIODevice::WriteOnly);
  return writeDibAsImage(dib, format, &buf) ? encoded : QByteArray{};
}

bool ClipboardImage::writeDibAsImage(const QByteArray &dib, const char *format, QIODevice *device)
{
  const auto bmpFile = dibToBmp(dib);
  if (bmpFile.isEmpty()) {
    LOG_WARN("clipboard bitmap data is malformed");
    return false;
  }

  const auto image = decode(bmpFile, "BMP");
  return !image.isNull() && write(image, format, device);
}

QByteArray ClipboardImage::imageToDib(const QByteArray &encoded, const char *format)
{
  const auto image = decode(encoded, format);
  QByteArray bmp;
  QBuffer buf(&bmp);
  buf.open(QIODevice::WriteOnly);
  return !image.isNull() && write(image, "BMP", &buf) ? bmpToDib(bmp) : QByteArray{};
}

QByteArray ClipboardImage::toPng(const QByteArray &encoded, const char *format)
{
  QByteArray png;
  QBuffer buf(&png);
  buf.open(QIODevice::WriteOnly);
  return writeAsPng(encoded, format, &buf) ? png : QByteArray{};
}

bool ClipboardImage::writeAsPng(const QByteArray &encoded, const char *format, QIODevice *device)
{
  const auto image = decode(encoded, format);
  return !image.isNull() && write(image, "PNG", device);
}

const char *ClipboardImage::qtFormat(IClipboard::Format format)
{
  const char *name = nullptr;
  if (format == IClipboard::Format::PNG)
    name = "PNG";
  else if (format == IClipboard::Format::JPEG)
    name = "JPEG";
  else if (format == IClipboard::Format::WebP)
    name = "WEBP";
  else if (format == IClipboard::Format::TIFF)
    name = "TIFF";
  return name;
}

QImage ClipboardImage::decode(const QByteArray &encoded, const char *format)
{
  QImageReader::setAllocationLimit(kMaxDecodeMegabytes);
  QImage image;
  if (!image.loadFromData(encoded, format))
    LOG_WARN("failed to decode clipboard image, format: %s", format);
  return image;
}

bool ClipboardImage::write(const QImage &image, const char *format, QIODevice *device)
{
  QImageWriter writer(device, format);
  if (qstricmp(format, "PNG") == 0)
    writer.setCompression(kPngCompression);

  const bool written = writer.write(image);
  if (!written)
    LOG_WARN("failed to encode clipboard image, format: %s", format);
  return written;
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

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/ClipboardImageConverter.h"

#include <QBuffer>
#include <QImage>

#include <limits>
#include <string_view>

namespace deskflow::platform::clipboard {
namespace {

constexpr size_t kBmpFileHeaderSize = 14;
constexpr size_t kBmpInfoHeaderSize = 40;

static inline uint32_t fromLEU32(const uint8_t *data)
{
  return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

static inline int32_t fromLES32(const uint8_t *data)
{
  return static_cast<int32_t>(
      static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) | (static_cast<uint32_t>(data[2]) << 16) |
      (static_cast<uint32_t>(data[3]) << 24)
  );
}

static inline uint16_t fromLEU16(const uint8_t *data)
{
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

static inline void toLE(uint8_t *&dst, char src)
{
  dst[0] = static_cast<uint8_t>(src);
  dst += 1;
}

static inline void toLE(uint8_t *&dst, uint16_t src)
{
  dst[0] = static_cast<uint8_t>(src & 0xffu);
  dst[1] = static_cast<uint8_t>((src >> 8) & 0xffu);
  dst += 2;
}

static inline void toLE(uint8_t *&dst, uint32_t src)
{
  dst[0] = static_cast<uint8_t>(src & 0xffu);
  dst[1] = static_cast<uint8_t>((src >> 8) & 0xffu);
  dst[2] = static_cast<uint8_t>((src >> 16) & 0xffu);
  dst[3] = static_cast<uint8_t>((src >> 24) & 0xffu);
  dst += 4;
}

std::string addBmpFileHeader(const std::string &bitmapData)
{
  if (bitmapData.size() < kBmpInfoHeaderSize) {
    return {};
  }

  const auto *rawInfoHeader = reinterpret_cast<const uint8_t *>(bitmapData.data());
  uint32_t infoHeaderSize = fromLEU32(rawInfoHeader);
  if (infoHeaderSize < kBmpInfoHeaderSize || infoHeaderSize > bitmapData.size()) {
    return {};
  }

  uint8_t header[kBmpFileHeaderSize];
  uint8_t *dst = header;
  toLE(dst, 'B');
  toLE(dst, 'M');
  toLE(dst, static_cast<uint32_t>(kBmpFileHeaderSize + bitmapData.size()));
  toLE(dst, static_cast<uint16_t>(0));
  toLE(dst, static_cast<uint16_t>(0));
  toLE(dst, static_cast<uint32_t>(kBmpFileHeaderSize + infoHeaderSize));

  return std::string(reinterpret_cast<const char *>(header), kBmpFileHeaderSize) + bitmapData;
}

std::string stripBmpFileHeader(const std::string &bmpData)
{
  if (bmpData.size() <= kBmpFileHeaderSize + kBmpInfoHeaderSize) {
    return {};
  }

  const auto *rawBmpHeader = reinterpret_cast<const uint8_t *>(bmpData.data());
  if (rawBmpHeader[0] != 'B' || rawBmpHeader[1] != 'M') {
    return {};
  }

  uint32_t offset = fromLEU32(rawBmpHeader + 10);
  if (offset <= kBmpFileHeaderSize || offset > bmpData.size()) {
    return {};
  }

  if (offset == kBmpFileHeaderSize + kBmpInfoHeaderSize) {
    return bmpData.substr(kBmpFileHeaderSize);
  }

  return bmpData.substr(kBmpFileHeaderSize, kBmpInfoHeaderSize) + bmpData.substr(offset, bmpData.size() - offset);
}

bool isTooLargeForQt(const std::string &data)
{
  return data.size() > static_cast<size_t>(std::numeric_limits<int>::max());
}

QImage imageFromDib(const std::string &bitmapData)
{
  if (bitmapData.size() < kBmpInfoHeaderSize) {
    return {};
  }

  const auto *rawInfoHeader = reinterpret_cast<const uint8_t *>(bitmapData.data());
  const auto infoHeaderSize = static_cast<size_t>(fromLEU32(rawInfoHeader + 0));
  const auto width = fromLES32(rawInfoHeader + 4);
  const auto signedHeight = fromLES32(rawInfoHeader + 8);
  const auto planes = fromLEU16(rawInfoHeader + 12);
  const auto bitCount = fromLEU16(rawInfoHeader + 14);
  const auto compression = fromLEU32(rawInfoHeader + 16);

  if (infoHeaderSize < kBmpInfoHeaderSize || infoHeaderSize > bitmapData.size()) {
    return {};
  }
  if (width <= 0 || signedHeight == 0 || planes != 1) {
    return {};
  }
  if (compression != 0 || (bitCount != 24 && bitCount != 32)) {
    return {};
  }

  const bool topDown = signedHeight < 0;
  const int32_t height = topDown ? -signedHeight : signedHeight;
  if (height <= 0) {
    return {};
  }

  size_t rowStride = 0;
  if (bitCount == 24) {
    // BMP rows are 4-byte aligned. Round width * 3 bytes up to the next 4-byte boundary.
    rowStride = static_cast<size_t>((width * 3 + 3) & ~3);
  } else {
    rowStride = static_cast<size_t>(width) * 4;
  }

  const auto pixelsOffset = infoHeaderSize;
  const auto requiredSize = pixelsOffset + rowStride * static_cast<size_t>(height);
  if (requiredSize > bitmapData.size()) {
    return {};
  }

  const auto imageFormat = bitCount == 32 ? QImage::Format_ARGB32 : QImage::Format_RGB888;
  QImage image(width, height, imageFormat);
  if (image.isNull()) {
    return {};
  }

  const auto *rawPixels = reinterpret_cast<const uint8_t *>(bitmapData.data()) + pixelsOffset;
  for (int y = 0; y < height; ++y) {
    const int srcRow = topDown ? y : (height - 1 - y);
    const auto *src = rawPixels + static_cast<size_t>(srcRow) * rowStride;
    if (bitCount == 24) {
      auto *dst = image.scanLine(y);
      for (int x = 0; x < width; ++x) {
        const auto srcPixel = src + x * 3;
        dst[x * 3 + 0] = srcPixel[2];
        dst[x * 3 + 1] = srcPixel[1];
        dst[x * 3 + 2] = srcPixel[0];
      }
    } else {
      auto *dst = reinterpret_cast<QRgb *>(image.scanLine(y));
      for (int x = 0; x < width; ++x) {
        const auto srcPixel = src + x * 4;
        dst[x] = qRgba(srcPixel[2], srcPixel[1], srcPixel[0], srcPixel[3]);
      }
    }
  }

  return image;
}

QImage imageFromData(const std::string &imageData, std::string_view formatHint)
{
  if (isTooLargeForQt(imageData)) {
    return {};
  }

  const auto *rawData = reinterpret_cast<const uchar *>(imageData.data());
  const auto size = static_cast<int>(imageData.size());

  if (!formatHint.empty()) {
    const QByteArray format{formatHint.data(), static_cast<int>(formatHint.size())};
    if (QImage image = QImage::fromData(rawData, size, format.constData()); !image.isNull()) {
      return image;
    }
  }

  return QImage::fromData(rawData, size);
}

bool hasOnlyZeroAlphaWithVisibleRgb(const QImage &image)
{
  bool sawVisibleColor = false;
  for (int y = 0; y < image.height(); ++y) {
    const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
    for (int x = 0; x < image.width(); ++x) {
      const auto pixel = row[x];
      if (qAlpha(pixel) != 0) {
        return false;
      }
      if (qRed(pixel) != 0 || qGreen(pixel) != 0 || qBlue(pixel) != 0) {
        sawVisibleColor = true;
      }
    }
  }

  return sawVisibleColor;
}

} // namespace

std::string encodeBitmapToImage(const std::string &bitmapData, const char *format)
{
  if (format == nullptr || *format == '\0') {
    return {};
  }

  auto image = imageFromDib(bitmapData);
  if (image.isNull()) {
    const auto bmpData = addBmpFileHeader(bitmapData);
    if (bmpData.empty()) {
      return {};
    }

    image = imageFromData(bmpData, "BMP");
    if (image.isNull()) {
      return {};
    }
  }

  auto saveImage = image.convertToFormat(QImage::Format_ARGB32);
  if (saveImage.isNull()) {
    return {};
  }

  if (hasOnlyZeroAlphaWithVisibleRgb(saveImage)) {
    // Some DIB clipboard sources use undefined alpha bytes and populate them with
    // 0x00 even when RGB channels are valid. Preserve colors for this specific case.
    for (int y = 0; y < saveImage.height(); ++y) {
      auto *row = reinterpret_cast<QRgb *>(saveImage.scanLine(y));
      for (int x = 0; x < saveImage.width(); ++x) {
        row[x] = qRgba(qRed(row[x]), qGreen(row[x]), qBlue(row[x]), 255);
      }
    }
  }

  QByteArray encoded;
  QBuffer buffer(&encoded);
  if (!buffer.open(QIODevice::WriteOnly)) {
    return {};
  }

  if (!saveImage.save(&buffer, format)) {
    return {};
  }

  return std::string(encoded.constData(), static_cast<size_t>(encoded.size()));
}

std::string decodeImageToBitmap(const std::string &imageData, const char *formatHint)
{
  const auto image = imageFromData(imageData, formatHint ? std::string_view(formatHint) : std::string_view());
  if (image.isNull()) {
    return {};
  }

  const auto normalized = image.convertToFormat(QImage::Format_RGB888);
  if (normalized.isNull()) {
    return {};
  }

  QByteArray bmpData;
  QBuffer buffer(&bmpData);
  if (!buffer.open(QIODevice::WriteOnly)) {
    return {};
  }

  if (!normalized.save(&buffer, "BMP")) {
    return {};
  }

  const auto bmp = std::string(bmpData.constData(), static_cast<size_t>(bmpData.size()));
  return stripBmpFileHeader(bmp);
}

} // namespace deskflow::platform::clipboard

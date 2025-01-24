/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardAnyBitmapConverter.h"

// BMP info header structure
struct CBMPInfoHeader
{
public:
  uint32_t biSize;
  int32_t biWidth;
  int32_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t biXPelsPerMeter;
  int32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
};

// BMP is little-endian

static void toLE(uint8_t *&dst, uint16_t src)
{
  dst[0] = static_cast<uint8_t>(src & 0xffu);
  dst[1] = static_cast<uint8_t>((src >> 8) & 0xffu);
  dst += 2;
}

static void toLE(uint8_t *&dst, int32_t src)
{
  dst[0] = static_cast<uint8_t>(src & 0xffu);
  dst[1] = static_cast<uint8_t>((src >> 8) & 0xffu);
  dst[2] = static_cast<uint8_t>((src >> 16) & 0xffu);
  dst[3] = static_cast<uint8_t>((src >> 24) & 0xffu);
  dst += 4;
}

static void toLE(uint8_t *&dst, uint32_t src)
{
  dst[0] = static_cast<uint8_t>(src & 0xffu);
  dst[1] = static_cast<uint8_t>((src >> 8) & 0xffu);
  dst[2] = static_cast<uint8_t>((src >> 16) & 0xffu);
  dst[3] = static_cast<uint8_t>((src >> 24) & 0xffu);
  dst += 4;
}

static inline uint16_t fromLEU16(const uint8_t *data)
{
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

static inline int32_t fromLES32(const uint8_t *data)
{
  return static_cast<int32_t>(
      static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) | (static_cast<uint32_t>(data[2]) << 16) |
      (static_cast<uint32_t>(data[3]) << 24)
  );
}

static inline uint32_t fromLEU32(const uint8_t *data)
{
  return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

//
// XWindowsClipboardAnyBitmapConverter
//

XWindowsClipboardAnyBitmapConverter::XWindowsClipboardAnyBitmapConverter()
{
  // do nothing
}

XWindowsClipboardAnyBitmapConverter::~XWindowsClipboardAnyBitmapConverter()
{
  // do nothing
}

IClipboard::EFormat XWindowsClipboardAnyBitmapConverter::getFormat() const
{
  return IClipboard::kBitmap;
}

int XWindowsClipboardAnyBitmapConverter::getDataSize() const
{
  return 8;
}

std::string XWindowsClipboardAnyBitmapConverter::fromIClipboard(const std::string &bmp) const
{
  // fill BMP info header with native-endian data
  CBMPInfoHeader infoHeader;
  const uint8_t *rawBMPInfoHeader = reinterpret_cast<const uint8_t *>(bmp.data());
  infoHeader.biSize = fromLEU32(rawBMPInfoHeader + 0);
  infoHeader.biWidth = fromLES32(rawBMPInfoHeader + 4);
  infoHeader.biHeight = fromLES32(rawBMPInfoHeader + 8);
  infoHeader.biPlanes = fromLEU16(rawBMPInfoHeader + 12);
  infoHeader.biBitCount = fromLEU16(rawBMPInfoHeader + 14);
  infoHeader.biCompression = fromLEU32(rawBMPInfoHeader + 16);
  infoHeader.biSizeImage = fromLEU32(rawBMPInfoHeader + 20);
  infoHeader.biXPelsPerMeter = fromLES32(rawBMPInfoHeader + 24);
  infoHeader.biYPelsPerMeter = fromLES32(rawBMPInfoHeader + 28);
  infoHeader.biClrUsed = fromLEU32(rawBMPInfoHeader + 32);
  infoHeader.biClrImportant = fromLEU32(rawBMPInfoHeader + 36);

  // check that format is acceptable
  if (infoHeader.biSize != 40 || infoHeader.biWidth == 0 || infoHeader.biHeight == 0 || infoHeader.biPlanes != 0 ||
      infoHeader.biCompression != 0 || (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32)) {
    return std::string();
  }

  // convert to image format
  const uint8_t *rawBMPPixels = rawBMPInfoHeader + 40;
  if (infoHeader.biBitCount == 24) {
    return doBGRFromIClipboard(rawBMPPixels, infoHeader.biWidth, infoHeader.biHeight);
  } else {
    return doBGRAFromIClipboard(rawBMPPixels, infoHeader.biWidth, infoHeader.biHeight);
  }
}

std::string XWindowsClipboardAnyBitmapConverter::toIClipboard(const std::string &image) const
{
  // convert to raw BMP data
  uint32_t w, h, depth;
  std::string rawBMP = doToIClipboard(image, w, h, depth);
  if (rawBMP.empty() || w == 0 || h == 0 || (depth != 24 && depth != 32)) {
    return std::string();
  }

  // fill BMP info header with little-endian data
  uint8_t infoHeader[40];
  uint8_t *dst = infoHeader;
  toLE(dst, static_cast<uint32_t>(40));
  toLE(dst, static_cast<int32_t>(w));
  toLE(dst, static_cast<int32_t>(h));
  toLE(dst, static_cast<uint16_t>(1));
  toLE(dst, static_cast<uint16_t>(depth));
  toLE(dst, static_cast<uint32_t>(0)); // BI_RGB
  toLE(dst, static_cast<uint32_t>(image.size()));
  toLE(dst, static_cast<int32_t>(2834)); // 72 dpi
  toLE(dst, static_cast<int32_t>(2834)); // 72 dpi
  toLE(dst, static_cast<uint32_t>(0));
  toLE(dst, static_cast<uint32_t>(0));

  // construct image
  return std::string(reinterpret_cast<const char *>(infoHeader), sizeof(infoHeader)) + rawBMP;
}

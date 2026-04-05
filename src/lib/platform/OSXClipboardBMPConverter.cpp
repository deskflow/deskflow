/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardBMPConverter.h"
#include "base/Log.h"

// BMP file header structure
struct CBMPHeader
{
public:
  uint16_t type;
  uint32_t size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t offset;
};

// BMP is little-endian
static inline uint16_t fromLEU16(const uint8_t *data)
{
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

static inline uint32_t fromLEU32(const uint8_t *data)
{
  return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

static void toLE(uint8_t *&dst, char src)
{
  dst[0] = static_cast<uint8_t>(src);
  dst += 1;
}

static void toLE(uint8_t *&dst, uint16_t src)
{
  dst[0] = static_cast<uint8_t>(src & 0xffu);
  dst[1] = static_cast<uint8_t>((src >> 8) & 0xffu);
  dst += 2;
}

static void toLE(uint8_t *&dst, uint32_t src)
{
  dst[0] = static_cast<uint8_t>(src & 0xffu);
  dst[1] = static_cast<uint8_t>((src >> 8) & 0xffu);
  dst[2] = static_cast<uint8_t>((src >> 16) & 0xffu);
  dst[3] = static_cast<uint8_t>((src >> 24) & 0xffu);
  dst += 4;
}

IClipboard::Format OSXClipboardBMPConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

CFStringRef OSXClipboardBMPConverter::getOSXFormat() const
{
  // TODO: does this only work with Windows?
  return CFSTR("com.microsoft.bmp");
}

std::string OSXClipboardBMPConverter::fromIClipboard(const std::string &bmp) const
{
  LOG_DEBUG1("getting data from clipboard");

  if (bmp.size() < 4) {
    return std::string();
  }

  // read the actual DIB header size from biSize
  const uint8_t *rawDIB = reinterpret_cast<const uint8_t *>(bmp.data());
  uint32_t biSize = fromLEU32(rawDIB);

  // compute pixel data offset: file header + DIB header + color table
  uint32_t pixelOffset = 14 + biSize;

  if (biSize >= 40 && bmp.size() >= 40) {
    uint16_t biBitCount = fromLEU16(rawDIB + 14);
    uint32_t biCompression = fromLEU32(rawDIB + 16);
    uint32_t biClrUsed = fromLEU32(rawDIB + 32);

    // BITMAPINFOHEADER with BI_BITFIELDS has 3 DWORD color masks after header
    if (biSize == 40 && biCompression == 3 /* BI_BITFIELDS */) {
      pixelOffset += 3 * 4;
    }

    // add color table size
    if (biBitCount <= 8) {
      uint32_t colors = biClrUsed ? biClrUsed : (1u << biBitCount);
      pixelOffset += colors * 4;
    } else if (biClrUsed > 0) {
      pixelOffset += biClrUsed * 4;
    }
  }

  // create BMP file header
  uint8_t header[14];
  uint8_t *dst = header;
  toLE(dst, 'B');
  toLE(dst, 'M');
  toLE(dst, static_cast<uint32_t>(14 + bmp.size()));
  toLE(dst, static_cast<uint16_t>(0));
  toLE(dst, static_cast<uint16_t>(0));
  toLE(dst, pixelOffset);
  return std::string(reinterpret_cast<const char *>(header), 14) + bmp;
}

std::string OSXClipboardBMPConverter::toIClipboard(const std::string &bmp) const
{
  // make sure data is big enough for a BMP file header + minimal DIB header
  if (bmp.size() <= 14 + 40) {
    return std::string();
  }

  // check BMP file header
  const uint8_t *rawBMPHeader = reinterpret_cast<const uint8_t *>(bmp.data());
  if (rawBMPHeader[0] != 'B' || rawBMPHeader[1] != 'M') {
    return std::string();
  }

  // strip the 14-byte BMP file header, keep the entire DIB
  // (info header + optional color table + pixel data)
  return bmp.substr(14);
}

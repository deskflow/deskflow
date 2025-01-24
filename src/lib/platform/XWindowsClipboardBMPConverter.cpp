/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardBMPConverter.h"

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

//
// XWindowsClipboardBMPConverter
//

XWindowsClipboardBMPConverter::XWindowsClipboardBMPConverter(Display *display)
    : m_atom(XInternAtom(display, "image/bmp", False))
{
  // do nothing
}

XWindowsClipboardBMPConverter::~XWindowsClipboardBMPConverter()
{
  // do nothing
}

IClipboard::EFormat XWindowsClipboardBMPConverter::getFormat() const
{
  return IClipboard::kBitmap;
}

Atom XWindowsClipboardBMPConverter::getAtom() const
{
  return m_atom;
}

int XWindowsClipboardBMPConverter::getDataSize() const
{
  return 8;
}

std::string XWindowsClipboardBMPConverter::fromIClipboard(const std::string &bmp) const
{
  // create BMP image
  uint8_t header[14];
  uint8_t *dst = header;
  toLE(dst, 'B');
  toLE(dst, 'M');
  toLE(dst, static_cast<uint32_t>(14 + bmp.size()));
  toLE(dst, static_cast<uint16_t>(0));
  toLE(dst, static_cast<uint16_t>(0));
  toLE(dst, static_cast<uint32_t>(14 + 40));
  return std::string(reinterpret_cast<const char *>(header), 14) + bmp;
}

std::string XWindowsClipboardBMPConverter::toIClipboard(const std::string &bmp) const
{
  // make sure data is big enough for a BMP file
  if (bmp.size() <= 14 + 40) {
    return std::string();
  }

  // check BMP file header
  const uint8_t *rawBMPHeader = reinterpret_cast<const uint8_t *>(bmp.data());
  if (rawBMPHeader[0] != 'B' || rawBMPHeader[1] != 'M') {
    return std::string();
  }

  // get offset to image data
  uint32_t offset = fromLEU32(rawBMPHeader + 10);

  // construct BMP
  if (offset == 14 + 40) {
    return bmp.substr(14);
  } else {
    return bmp.substr(14, 40) + bmp.substr(offset, bmp.size() - offset);
  }
}

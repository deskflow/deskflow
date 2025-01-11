/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/XWindowsClipboardBMPConverter.h"

// BMP file header structure
struct CBMPHeader
{
public:
  uint16_t type;
  UInt32 size;
  uint16_t reserved1;
  uint16_t reserved2;
  UInt32 offset;
};

// BMP is little-endian
static inline UInt32 fromLEU32(const uint8_t *data)
{
  return static_cast<UInt32>(data[0]) | (static_cast<UInt32>(data[1]) << 8) | (static_cast<UInt32>(data[2]) << 16) |
         (static_cast<UInt32>(data[3]) << 24);
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

static void toLE(uint8_t *&dst, UInt32 src)
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
  toLE(dst, static_cast<UInt32>(14 + bmp.size()));
  toLE(dst, static_cast<uint16_t>(0));
  toLE(dst, static_cast<uint16_t>(0));
  toLE(dst, static_cast<UInt32>(14 + 40));
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
  UInt32 offset = fromLEU32(rawBMPHeader + 10);

  // construct BMP
  if (offset == 14 + 40) {
    return bmp.substr(14);
  } else {
    return bmp.substr(14, 40) + bmp.substr(offset, bmp.size() - offset);
  }
}

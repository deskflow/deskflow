/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClipboardBitmapTests.h"

#include "deskflow/ClipboardBitmap.h"

#include <QtEndian>

using namespace deskflow::bmp;

namespace {
constexpr quint32 kFileHeaderSize = 14;
constexpr quint32 kBiBitfields = 3;

// DIB header of headerSize bytes (40 = BITMAPINFOHEADER, 124 = BITMAPV5HEADER) for a 1x1 image
std::string dibHeader(quint32 headerSize, quint16 bitCount, quint32 compression)
{
  std::string header(headerSize, '\0');
  auto *raw = reinterpret_cast<quint8 *>(header.data());
  qToLittleEndian<quint32>(headerSize, raw);
  qToLittleEndian<qint32>(1, raw + 4);
  qToLittleEndian<qint32>(1, raw + 8);
  qToLittleEndian<quint16>(1, raw + 12);
  qToLittleEndian<quint16>(bitCount, raw + 14);
  qToLittleEndian<quint32>(compression, raw + 16);
  return header;
}

std::string fileHeader(quint32 totalSize, quint32 pixelOffset)
{
  std::string header(kFileHeaderSize, '\0');
  auto *raw = reinterpret_cast<quint8 *>(header.data());
  raw[0] = 'B';
  raw[1] = 'M';
  qToLittleEndian<quint32>(totalSize, raw + 2);
  qToLittleEndian<quint32>(pixelOffset, raw + 10);
  return header;
}

quint32 pixelOffsetOf(const std::string &bmp)
{
  return qFromLittleEndian<quint32>(reinterpret_cast<const quint8 *>(bmp.data()) + 10);
}

const std::string kMasks("RRRRGGGGBBBB");
const std::string kPixel("\x11\x22\x33\x44", 4);
} // namespace

void ClipboardBitmapTests::bmpToDib_bitfieldsHeader_keepsMasks()
{
  // #9822: a 40-byte header with BI_BITFIELDS is followed by 12 bytes of colour masks
  const std::string dib = dibHeader(40, 32, kBiBitfields) + kMasks + kPixel;
  const std::string bmp = fileHeader(kFileHeaderSize + dib.size(), kFileHeaderSize + 40 + 12) + dib;

  QCOMPARE(bmpToDib(bmp), dib);
}

void ClipboardBitmapTests::bmpToDib_v5HeaderWithGap_dropsOnlyTheGap()
{
  const std::string header = dibHeader(124, 32, kBiBitfields);
  const std::string gap(68, '\x7f');
  const quint32 offset = kFileHeaderSize + header.size() + gap.size();
  const std::string bmp = fileHeader(offset + kPixel.size(), offset) + header + gap + kPixel;

  QCOMPARE(bmpToDib(bmp), header + kPixel);
}

void ClipboardBitmapTests::bmpToDib_notBmp_isRejected()
{
  QCOMPARE(bmpToDib("XX" + std::string(60, '\0')), std::string());
  QCOMPARE(bmpToDib("BM"), std::string());
}

void ClipboardBitmapTests::dibToBmp_v5Header_pointsPastHeader()
{
  const std::string dib = dibHeader(124, 32, kBiBitfields) + kPixel;

  const std::string bmp = dibToBmp(dib);

  QCOMPARE(bmp.substr(0, 2), std::string("BM"));
  QCOMPARE(pixelOffsetOf(bmp), kFileHeaderSize + 124);
  QCOMPARE(bmp.substr(kFileHeaderSize), dib);
}

void ClipboardBitmapTests::dibToBmp_palette_pointsPastPalette()
{
  // 8-bit with biClrUsed = 0 has a full 256-entry palette
  const std::string dib = dibHeader(40, 8, 0) + std::string(256 * 4, '\0') + kPixel;

  QCOMPARE(pixelOffsetOf(dibToBmp(dib)), kFileHeaderSize + 40 + 256 * 4);
}

void ClipboardBitmapTests::roundTrip_v5Header_isUnchanged()
{
  const std::string dib = dibHeader(124, 32, kBiBitfields) + kPixel;

  QCOMPARE(bmpToDib(dibToBmp(dib)), dib);
}

void ClipboardBitmapTests::roundTrip_plainHeader_isUnchanged()
{
  // the common case on X11: 40-byte header, 24-bit, pixels right after the header
  const std::string dib = dibHeader(40, 24, 0) + kPixel;
  const std::string bmp = dibToBmp(dib);

  QCOMPARE(pixelOffsetOf(bmp), kFileHeaderSize + 40);
  QCOMPARE(bmpToDib(bmp), dib);
}

void ClipboardBitmapTests::dibPixelOffset_paletteLargerThanData_isInvalid()
{
  // clipboard data comes from another computer: a palette count of 0x40000000 * 4 bytes must not wrap to 0
  std::string dib = dibHeader(40, 8, 0) + kPixel;
  qToLittleEndian<quint32>(0x40000000u, reinterpret_cast<quint8 *>(dib.data()) + 32);

  QCOMPARE(dibPixelOffset(reinterpret_cast<const quint8 *>(dib.data()), dib.size()), 0u);
}

QTEST_GUILESS_MAIN(ClipboardBitmapTests)

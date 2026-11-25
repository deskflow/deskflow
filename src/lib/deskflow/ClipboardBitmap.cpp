/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2014 - 2016, 2023 - 2026 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ClipboardBitmap.h"

#include "base/Log.h"

#include <QtEndian>

namespace {
constexpr quint32 kBiBitfields = 3;
constexpr quint32 kBiAlphabitfields = 6;
constexpr quint32 kBmpHeaderDIBPad = 10;
constexpr quint32 kBmpFileHeaderSize = 14;
constexpr quint32 kFallbackPixelOffset = 40;
} // namespace

namespace deskflow::bmp {

quint32 dibPixelOffset(const quint8 *dib, qsizetype dibSize)
{
  // 64-bit so that header values from the other computer cannot wrap the offset
  quint64 pixelOffset = 0;
  if (dibSize >= 16) {
    const auto biSize = qFromLittleEndian<quint32>(dib);
    if (biSize >= 12 && biSize <= dibSize) {
      pixelOffset = biSize;
      if (biSize >= kFallbackPixelOffset) {
        const auto biBitCount = qFromLittleEndian<quint16>(dib + 14);
        const auto biCompression = qFromLittleEndian<quint32>(dib + 16);
        if (biSize == kFallbackPixelOffset) {
          if (biCompression == kBiBitfields) {
            pixelOffset += 12;
          } else if (biCompression == kBiAlphabitfields) {
            pixelOffset += 16;
          }
        }
        if (biBitCount > 0 && biBitCount <= 8) {
          const auto biClrUsed = qFromLittleEndian<quint32>(dib + 32);
          const quint64 numColors = biClrUsed != 0 ? biClrUsed : (1u << biBitCount);
          pixelOffset += numColors * 4;
        }
      }
    }
  }
  if (pixelOffset > static_cast<quint64>(dibSize)) {
    return 0;
  }
  return static_cast<quint32>(pixelOffset);
}

std::string dibToBmp(const std::string &dib)
{
  if (dib.size() < 4) {
    LOG_DEBUG("rejecting clipboard dib, too small to wrap as bmp, size: %zu bytes", dib.size());
    return std::string();
  }

  auto pixelOffset = dibPixelOffset(reinterpret_cast<const quint8 *>(dib.data()), dib.size());
  if (pixelOffset == 0) {
    pixelOffset = kFallbackPixelOffset;
  }

  quint8 header[kBmpFileHeaderSize];
  header[0] = 'B';
  header[1] = 'M';
  qToLittleEndian<quint32>(static_cast<quint32>(kBmpFileHeaderSize + dib.size()), header + 2);
  qToLittleEndian<quint16>(0, header + 6);
  qToLittleEndian<quint16>(0, header + 8);
  qToLittleEndian<quint32>(kBmpFileHeaderSize + pixelOffset, header + kBmpHeaderDIBPad);
  return std::string(reinterpret_cast<const char *>(header), kBmpFileHeaderSize) + dib;
}

std::string bmpToDib(const std::string &bmp)
{
  if (bmp.size() <= kBmpFileHeaderSize) {
    LOG_DEBUG("rejecting clipboard bmp, too small to parse, size: %zu bytes", bmp.size());
    return std::string();
  }
  const auto *raw = reinterpret_cast<const quint8 *>(bmp.data());
  if (raw[0] != 'B' || raw[1] != 'M') {
    LOG_DEBUG("rejecting clipboard bmp, missing bm magic");
    return std::string();
  }

  // the full DIB (extended header, bitfield masks, palette) must reach the receiver; truncating
  // to 40 bytes drops the masks of 32-bit BITMAPV5HEADER + BI_BITFIELDS images
  const auto offset = qFromLittleEndian<quint32>(raw + kBmpHeaderDIBPad);
  const auto naturalOffset = dibPixelOffset(raw + kBmpFileHeaderSize, bmp.size() - kBmpFileHeaderSize);
  const bool hasGap =
      naturalOffset != 0 && quint64{offset} > quint64{kBmpFileHeaderSize} + naturalOffset && offset <= bmp.size();
  if (hasGap) {
    return bmp.substr(kBmpFileHeaderSize, naturalOffset) + bmp.substr(offset);
  }
  return bmp.substr(kBmpFileHeaderSize);
}

} // namespace deskflow::bmp

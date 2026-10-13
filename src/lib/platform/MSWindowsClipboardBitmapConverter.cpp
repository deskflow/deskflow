/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardBitmapConverter.h"

#include "base/Log.h"

#include <QtEndian>

#include <cstdlib>
#include <limits>

namespace {
bool normaliseMacDib(const std::string &data, std::string &normalisedData)
{
  if (data.size() < sizeof(BITMAPINFOHEADER)) {
    return false;
  }

  const auto *header = reinterpret_cast<const BITMAPINFOHEADER *>(data.data());
  if (header->biWidth <= 0) {
    return false;
  }

  const auto width = static_cast<size_t>(header->biWidth);
  const auto height = static_cast<size_t>(std::abs(static_cast<int64_t>(header->biHeight)));
  if (height == 0 || width > (std::numeric_limits<size_t>::max() - sizeof(BITMAPINFOHEADER)) / 4 / height) {
    return false;
  }
  const auto pixelSize = width * height * 4;
  if (header->biPlanes != 1 || header->biBitCount != 32 || header->biCompression != BI_BITFIELDS) {
    return false;
  }

  size_t pixelOffset;
  if (header->biSize == sizeof(BITMAPV5HEADER) && data.size() >= sizeof(BITMAPV5HEADER) &&
      data.size() - sizeof(BITMAPV5HEADER) == pixelSize) {
    const auto *v5 = reinterpret_cast<const BITMAPV5HEADER *>(data.data());
    if (v5->bV5RedMask != 0x00ff0000 || v5->bV5GreenMask != 0x0000ff00 || v5->bV5BlueMask != 0x000000ff ||
        v5->bV5AlphaMask != 0xff000000 || v5->bV5CSType != LCS_sRGB || v5->bV5ClrUsed != 0 || v5->bV5ProfileData != 0 ||
        v5->bV5ProfileSize != 0) {
      return false;
    }
    // Windows clipboard consumers can fail to read a complete macOS V5 DIB
    // published as CF_DIB. Its sRGB BGRA pixels also fit a BI_RGB INFOHEADER.
    pixelOffset = sizeof(BITMAPV5HEADER);
  } else if (header->biSize > sizeof(BITMAPINFOHEADER) && data.size() == sizeof(BITMAPINFOHEADER) + pixelSize) {
    // Some macOS DIBs claim a V5 header but contain only an INFOHEADER.
    pixelOffset = sizeof(BITMAPINFOHEADER);
  } else {
    return false;
  }

  normalisedData = data.substr(0, sizeof(BITMAPINFOHEADER));
  qToLittleEndian<quint32>(sizeof(BITMAPINFOHEADER), reinterpret_cast<quint8 *>(&normalisedData[0]));
  qToLittleEndian<quint32>(BI_RGB, reinterpret_cast<quint8 *>(&normalisedData[0]) + 16);
  if (header->biHeight < 0) {
    // CF_DIB to CF_BITMAP synthesis requires a bottom-up DIB on Windows.
    if (height > static_cast<size_t>(std::numeric_limits<LONG>::max())) {
      return false;
    }
    qToLittleEndian<quint32>(static_cast<quint32>(height), reinterpret_cast<quint8 *>(normalisedData.data()) + 8);
    const auto stride = width * 4;
    for (size_t row = height; row > 0; --row) {
      normalisedData.append(data, pixelOffset + (row - 1) * stride, stride);
    }
  } else {
    normalisedData += data.substr(pixelOffset);
  }
  LOG_INFO("normalised macOS clipboard image to bottom-up BI_RGB");
  return true;
}
} // namespace

//
// MSWindowsClipboardBitmapConverter
//

IClipboard::Format MSWindowsClipboardBitmapConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

UINT MSWindowsClipboardBitmapConverter::getWin32Format() const
{
  return CF_DIB;
}

HANDLE
MSWindowsClipboardBitmapConverter::fromIClipboard(const std::string &data) const
{
  std::string normalisedData;
  const auto *clipboardData = &data;
  if (normaliseMacDib(data, normalisedData)) {
    clipboardData = &normalisedData;
  }

  // copy to memory handle
  HGLOBAL gData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, clipboardData->size());
  if (gData != nullptr) {
    // get a pointer to the allocated memory
    char *dst = (char *)GlobalLock(gData);
    if (dst != nullptr) {
      memcpy(dst, clipboardData->data(), clipboardData->size());
      GlobalUnlock(gData);
    } else {
      GlobalFree(gData);
      gData = nullptr;
    }
  }

  return gData;
}

std::string MSWindowsClipboardBitmapConverter::toIClipboard(HANDLE data) const
{
  // get datator
  LPVOID src = GlobalLock(data);
  if (src == nullptr) {
    return std::string();
  }
  uint32_t srcSize = (uint32_t)GlobalSize(data);

  // check image type
  const BITMAPINFO *bitmap = static_cast<const BITMAPINFO *>(src);
  LOG(
      (CLOG_INFO "bitmap: %dx%d %d", bitmap->bmiHeader.biWidth, bitmap->bmiHeader.biHeight,
       (int)bitmap->bmiHeader.biBitCount)
  );
  if (bitmap->bmiHeader.biPlanes == 1 && (bitmap->bmiHeader.biBitCount == 24 || bitmap->bmiHeader.biBitCount == 32) &&
      bitmap->bmiHeader.biCompression == BI_RGB) {
    // already in canonical form
    std::string image(static_cast<char const *>(src), srcSize);
    GlobalUnlock(data);
    return image;
  }

  // create a destination DIB section
  LOG_INFO("convert image from: depth=%d comp=%d", bitmap->bmiHeader.biBitCount, bitmap->bmiHeader.biCompression);
  void *raw;
  BITMAPINFOHEADER info;
  LONG w = bitmap->bmiHeader.biWidth;
  LONG h = bitmap->bmiHeader.biHeight;
  const LONG absHeight = (h < 0) ? -h : h;
  info.biSize = sizeof(BITMAPINFOHEADER);
  info.biWidth = w;
  info.biHeight = h;
  info.biPlanes = 1;
  info.biBitCount = 32;
  info.biCompression = BI_RGB;
  info.biSizeImage = 0;
  info.biXPelsPerMeter = 1000;
  info.biYPelsPerMeter = 1000;
  info.biClrUsed = 0;
  info.biClrImportant = 0;
  HDC dc = GetDC(nullptr);
  HBITMAP dst = CreateDIBSection(dc, (BITMAPINFO *)&info, DIB_RGB_COLORS, &raw, nullptr, 0);
  if (dst == nullptr || raw == nullptr) {
    LOG_WARN("failed to allocate destination bitmap for clipboard image");
    ReleaseDC(nullptr, dc);
    GlobalUnlock(data);
    return std::string();
  }

  // find the start of the pixel data
  const char *srcBits = (const char *)bitmap + bitmap->bmiHeader.biSize;
  if (bitmap->bmiHeader.biBitCount >= 16) {
    if (bitmap->bmiHeader.biCompression == BI_BITFIELDS &&
        (bitmap->bmiHeader.biBitCount == 16 || bitmap->bmiHeader.biBitCount == 32)) {
      srcBits += 3 * sizeof(DWORD);
    }
  } else if (bitmap->bmiHeader.biClrUsed != 0) {
    srcBits += bitmap->bmiHeader.biClrUsed * sizeof(RGBQUAD);
  } else {
    // http://msdn.microsoft.com/en-us/library/ke55d167(VS.80).aspx
    srcBits += (1i64 << bitmap->bmiHeader.biBitCount) * sizeof(RGBQUAD);
  }

  // copy source image to destination image
  HDC dstDC = CreateCompatibleDC(dc);
  HGDIOBJ oldBitmap = SelectObject(dstDC, dst);
  SetDIBitsToDevice(dstDC, 0, 0, w, absHeight, 0, 0, 0, absHeight, srcBits, bitmap, DIB_RGB_COLORS);
  SelectObject(dstDC, oldBitmap);
  DeleteDC(dstDC);
  GdiFlush();

  // extract data
  std::string image((const char *)&info, info.biSize);
  image.append((const char *)raw, static_cast<size_t>(4) * static_cast<size_t>(w) * static_cast<size_t>(absHeight));

  // clean up GDI
  DeleteObject(dst);
  ReleaseDC(nullptr, dc);

  // release handle
  GlobalUnlock(data);

  return image;
}

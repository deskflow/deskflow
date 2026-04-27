/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardImageConverter.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace {

constexpr size_t kBitmapInfoHeaderSize = 40;
constexpr uint16_t kBmpBitsPerPixel = 24;
constexpr uint16_t kDibPlanes = 1;
constexpr uint32_t kDibCompressionRgb = 0;
constexpr int32_t kPixelsPerMeter = 1000;

CGBitmapInfo rgbaBitmapInfo()
{
  return static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedLast) |
         static_cast<CGBitmapInfo>(kCGBitmapByteOrder32Big);
}

uint16_t readU16LE(const char *data)
{
  uint16_t value;
  std::memcpy(&value, data, sizeof(value));
  return CFSwapInt16LittleToHost(value);
}

uint32_t readU32LE(const char *data)
{
  uint32_t value;
  std::memcpy(&value, data, sizeof(value));
  return CFSwapInt32LittleToHost(value);
}

int32_t readS32LE(const char *data)
{
  return static_cast<int32_t>(readU32LE(data));
}

void appendU16LE(std::string &data, uint16_t value)
{
  const auto littleEndian = CFSwapInt16HostToLittle(value);
  data.append(reinterpret_cast<const char *>(&littleEndian), sizeof(littleEndian));
}

void appendU32LE(std::string &data, uint32_t value)
{
  const auto littleEndian = CFSwapInt32HostToLittle(value);
  data.append(reinterpret_cast<const char *>(&littleEndian), sizeof(littleEndian));
}

void appendS32LE(std::string &data, int32_t value)
{
  appendU32LE(data, static_cast<uint32_t>(value));
}

bool checkedMul(size_t left, size_t right, size_t &result)
{
  if (left != 0 && right > std::numeric_limits<size_t>::max() / left) {
    return false;
  }

  result = left * right;
  return true;
}

bool dibRowStride(int32_t width, uint16_t bitCount, size_t &stride)
{
  if (width <= 0) {
    return false;
  }

  const auto bitsPerRow = static_cast<size_t>(width) * bitCount;
  stride = ((bitsPerRow + 31) / 32) * 4;
  return true;
}

std::string makeBitmapInfoHeader(int32_t width, int32_t height, uint32_t imageSize)
{
  std::string header;
  header.reserve(kBitmapInfoHeaderSize);
  appendU32LE(header, static_cast<uint32_t>(kBitmapInfoHeaderSize));
  appendS32LE(header, width);
  appendS32LE(header, height);
  appendU16LE(header, kDibPlanes);
  appendU16LE(header, kBmpBitsPerPixel);
  appendU32LE(header, kDibCompressionRgb);
  appendU32LE(header, imageSize);
  appendS32LE(header, kPixelsPerMeter);
  appendS32LE(header, kPixelsPerMeter);
  appendU32LE(header, 0);
  appendU32LE(header, 0);
  return header;
}

CGImageRef createImageFromData(const std::string &data)
{
  if (data.empty() || data.size() > static_cast<size_t>(std::numeric_limits<CFIndex>::max())) {
    return nullptr;
  }

  auto imageData = CFDataCreate(
      kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data.data()), static_cast<CFIndex>(data.size())
  );
  if (imageData == nullptr) {
    return nullptr;
  }

  auto source = CGImageSourceCreateWithData(imageData, nullptr);
  CFRelease(imageData);
  if (source == nullptr) {
    return nullptr;
  }

  auto image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
  CFRelease(source);
  return image;
}

bool drawImageToRgba(CGImageRef image, std::vector<uint8_t> &rgba, int32_t &width, int32_t &height)
{
  const auto imageWidth = CGImageGetWidth(image);
  const auto imageHeight = CGImageGetHeight(image);
  if (imageWidth == 0 || imageHeight == 0 || imageWidth > static_cast<size_t>(std::numeric_limits<int32_t>::max()) ||
      imageHeight > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
    return false;
  }

  width = static_cast<int32_t>(imageWidth);
  height = static_cast<int32_t>(imageHeight);

  size_t pixelCount;
  if (!checkedMul(static_cast<size_t>(width), static_cast<size_t>(height), pixelCount)) {
    return false;
  }

  size_t byteCount;
  if (!checkedMul(pixelCount, static_cast<size_t>(4), byteCount)) {
    return false;
  }

  rgba.assign(byteCount, 0);

  auto colorSpace = CGColorSpaceCreateDeviceRGB();
  if (colorSpace == nullptr) {
    return false;
  }

  auto context = CGBitmapContextCreate(
      rgba.data(), width, height, 8, static_cast<size_t>(width) * 4, colorSpace, rgbaBitmapInfo()
  );
  CGColorSpaceRelease(colorSpace);
  if (context == nullptr) {
    return false;
  }

  const auto rect = CGRectMake(0, 0, width, height);
  CGContextSetRGBFillColor(context, 1, 1, 1, 1);
  CGContextFillRect(context, rect);
  CGContextDrawImage(context, rect, image);
  CGContextRelease(context);
  return true;
}

std::string convertImageToDib(CGImageRef image)
{
  std::vector<uint8_t> rgba;
  int32_t width = 0;
  int32_t height = 0;
  if (!drawImageToRgba(image, rgba, width, height)) {
    return {};
  }

  size_t rowStride;
  if (!dibRowStride(width, kBmpBitsPerPixel, rowStride)) {
    return {};
  }

  size_t pixelDataSize;
  if (!checkedMul(rowStride, static_cast<size_t>(height), pixelDataSize) ||
      pixelDataSize > std::numeric_limits<uint32_t>::max()) {
    return {};
  }

  auto dib = makeBitmapInfoHeader(width, height, static_cast<uint32_t>(pixelDataSize));
  dib.resize(kBitmapInfoHeaderSize + pixelDataSize, '\0');

  auto *dstPixels = reinterpret_cast<uint8_t *>(dib.data() + kBitmapInfoHeaderSize);
  for (int32_t y = 0; y < height; ++y) {
    const auto *srcRow = rgba.data() + static_cast<size_t>(y) * static_cast<size_t>(width) * 4;
    auto *dstRow = dstPixels + static_cast<size_t>(height - 1 - y) * rowStride;
    for (int32_t x = 0; x < width; ++x) {
      const auto *src = srcRow + static_cast<size_t>(x) * 4;
      auto *dst = dstRow + static_cast<size_t>(x) * 3;
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
    }
  }

  return dib;
}

std::string convertImageDataToDib(const std::string &data)
{
  auto image = createImageFromData(data);
  if (image == nullptr) {
    return {};
  }

  auto dib = convertImageToDib(image);
  CGImageRelease(image);
  return dib;
}

struct DibInfo
{
  int32_t width = 0;
  int32_t height = 0;
  uint16_t bitCount = 0;
  bool topDown = false;
  size_t headerSize = 0;
  size_t rowStride = 0;
  const uint8_t *pixels = nullptr;
};

bool parseDib(const std::string &dib, DibInfo &info)
{
  if (dib.size() < kBitmapInfoHeaderSize) {
    return false;
  }

  const auto headerSize = readU32LE(dib.data());
  const auto width = readS32LE(dib.data() + 4);
  const auto signedHeight = readS32LE(dib.data() + 8);
  const auto planes = readU16LE(dib.data() + 12);
  const auto bitCount = readU16LE(dib.data() + 14);
  const auto compression = readU32LE(dib.data() + 16);
  if (headerSize < kBitmapInfoHeaderSize || headerSize > dib.size() || width <= 0 || signedHeight == 0 ||
      signedHeight == std::numeric_limits<int32_t>::min() || planes != 1 || compression != kDibCompressionRgb ||
      (bitCount != 24 && bitCount != 32)) {
    return false;
  }

  info.width = width;
  info.height = signedHeight < 0 ? -signedHeight : signedHeight;
  info.bitCount = bitCount;
  info.topDown = signedHeight < 0;
  info.headerSize = headerSize;
  if (!dibRowStride(info.width, info.bitCount, info.rowStride)) {
    return false;
  }

  size_t pixelDataSize;
  if (!checkedMul(info.rowStride, static_cast<size_t>(info.height), pixelDataSize) ||
      info.headerSize > dib.size() || pixelDataSize > dib.size() - info.headerSize) {
    return false;
  }

  info.pixels = reinterpret_cast<const uint8_t *>(dib.data() + info.headerSize);
  return true;
}

bool copyDibToRgba(const std::string &dib, std::vector<uint8_t> &rgba, int32_t &width, int32_t &height)
{
  DibInfo info;
  if (!parseDib(dib, info)) {
    return false;
  }

  size_t pixelCount;
  if (!checkedMul(static_cast<size_t>(info.width), static_cast<size_t>(info.height), pixelCount)) {
    return false;
  }

  size_t byteCount;
  if (!checkedMul(pixelCount, static_cast<size_t>(4), byteCount)) {
    return false;
  }

  rgba.assign(byteCount, 0);
  bool sawVisibleRgb = false;
  bool sawNonZeroAlpha = false;
  for (int32_t y = 0; y < info.height; ++y) {
    const int32_t srcY = info.topDown ? y : (info.height - 1 - y);
    const auto *srcRow = info.pixels + static_cast<size_t>(srcY) * info.rowStride;
    auto *dstRow = rgba.data() + static_cast<size_t>(y) * static_cast<size_t>(info.width) * 4;
    for (int32_t x = 0; x < info.width; ++x) {
      const auto *src = srcRow + static_cast<size_t>(x) * (info.bitCount / 8);
      auto *dst = dstRow + static_cast<size_t>(x) * 4;
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = info.bitCount == 32 ? src[3] : 255;
      sawVisibleRgb = sawVisibleRgb || dst[0] != 0 || dst[1] != 0 || dst[2] != 0;
      sawNonZeroAlpha = sawNonZeroAlpha || dst[3] != 0;
    }
  }

  if (info.bitCount == 32 && sawVisibleRgb && !sawNonZeroAlpha) {
    for (size_t i = 3; i < rgba.size(); i += 4) {
      rgba[i] = 255;
    }
  }

  for (size_t i = 0; i < rgba.size(); i += 4) {
    const auto alpha = rgba[i + 3];
    if (alpha != 0 && alpha != 255) {
      rgba[i + 0] = static_cast<uint8_t>((static_cast<uint16_t>(rgba[i + 0]) * alpha) / 255);
      rgba[i + 1] = static_cast<uint8_t>((static_cast<uint16_t>(rgba[i + 1]) * alpha) / 255);
      rgba[i + 2] = static_cast<uint8_t>((static_cast<uint16_t>(rgba[i + 2]) * alpha) / 255);
    }
  }

  width = info.width;
  height = info.height;
  return true;
}

CGImageRef createImageFromDib(const std::string &dib, std::vector<uint8_t> &rgbaStorage)
{
  int32_t width = 0;
  int32_t height = 0;
  if (!copyDibToRgba(dib, rgbaStorage, width, height)) {
    return nullptr;
  }

  auto colorSpace = CGColorSpaceCreateDeviceRGB();
  if (colorSpace == nullptr) {
    return nullptr;
  }

  auto context = CGBitmapContextCreate(
      rgbaStorage.data(), width, height, 8, static_cast<size_t>(width) * 4, colorSpace, rgbaBitmapInfo()
  );
  CGColorSpaceRelease(colorSpace);
  if (context == nullptr) {
    return nullptr;
  }

  auto image = CGBitmapContextCreateImage(context);
  CGContextRelease(context);
  return image;
}

std::string encodeImage(CGImageRef image, CFStringRef imageType)
{
  auto encoded = CFDataCreateMutable(kCFAllocatorDefault, 0);
  if (encoded == nullptr) {
    return {};
  }

  auto destination = CGImageDestinationCreateWithData(encoded, imageType, 1, nullptr);
  if (destination == nullptr) {
    CFRelease(encoded);
    return {};
  }

  CGImageDestinationAddImage(destination, image, nullptr);
  const bool ok = CGImageDestinationFinalize(destination);
  CFRelease(destination);
  if (!ok) {
    CFRelease(encoded);
    return {};
  }

  std::string result(
      reinterpret_cast<const char *>(CFDataGetBytePtr(encoded)), static_cast<size_t>(CFDataGetLength(encoded))
  );
  CFRelease(encoded);
  return result;
}

std::string convertDibToImageData(const std::string &dib, CFStringRef imageType)
{
  std::vector<uint8_t> rgbaStorage;
  auto image = createImageFromDib(dib, rgbaStorage);
  if (image == nullptr) {
    return {};
  }

  auto encoded = encodeImage(image, imageType);
  CGImageRelease(image);
  return encoded;
}

} // namespace

OSXClipboardImageConverter::OSXClipboardImageConverter(CFStringRef osxFormat, CFStringRef imageType)
    : m_osxFormat(osxFormat),
      m_imageType(imageType)
{
}

IClipboard::Format OSXClipboardImageConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

CFStringRef OSXClipboardImageConverter::getOSXFormat() const
{
  return m_osxFormat;
}

std::string OSXClipboardImageConverter::fromIClipboard(const std::string &data) const
{
  return convertDibToImageData(data, m_imageType);
}

std::string OSXClipboardImageConverter::toIClipboard(const std::string &data) const
{
  return convertImageDataToDib(data);
}

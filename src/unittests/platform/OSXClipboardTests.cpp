/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXClipboardTests.h"

#include "platform/OSXClipboard.h"
#include "platform/OSXClipboardImageConverter.h"
#include "platform/OSXClipboardUTF8Converter.h"

#include <CoreFoundation/CoreFoundation.h>

#include <cstring>

namespace {

constexpr size_t kBitmapInfoHeaderSize = 40;

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

std::string makeBitmapInfoHeader(int32_t width, int32_t height, uint16_t bitCount, uint32_t imageSize)
{
  std::string header;
  header.reserve(kBitmapInfoHeaderSize);
  appendU32LE(header, static_cast<uint32_t>(kBitmapInfoHeaderSize));
  appendS32LE(header, width);
  appendS32LE(header, height);
  appendU16LE(header, 1);
  appendU16LE(header, bitCount);
  appendU32LE(header, 0);
  appendU32LE(header, imageSize);
  appendS32LE(header, 1000);
  appendS32LE(header, 1000);
  appendU32LE(header, 0);
  appendU32LE(header, 0);
  return header;
}

void appendBgr(std::string &dib, uint8_t red, uint8_t green, uint8_t blue)
{
  dib.push_back(static_cast<char>(blue));
  dib.push_back(static_cast<char>(green));
  dib.push_back(static_cast<char>(red));
}

std::string make24BitDib(uint8_t red, uint8_t green, uint8_t blue)
{
  auto dib = makeBitmapInfoHeader(1, 1, 24, 4);
  appendBgr(dib, red, green, blue);
  dib.push_back('\0');
  return dib;
}

std::string make24BitDib2x2()
{
  auto dib = makeBitmapInfoHeader(2, 2, 24, 16);
  appendBgr(dib, 0x00, 0x00, 0xff);
  appendBgr(dib, 0xff, 0xff, 0xff);
  dib.append(2, '\0');
  appendBgr(dib, 0xff, 0x00, 0x00);
  appendBgr(dib, 0x00, 0xff, 0x00);
  dib.append(2, '\0');
  return dib;
}

std::string make32BitDib(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
  auto dib = makeBitmapInfoHeader(1, 1, 32, 4);
  dib.push_back(static_cast<char>(blue));
  dib.push_back(static_cast<char>(green));
  dib.push_back(static_cast<char>(red));
  dib.push_back(static_cast<char>(alpha));
  return dib;
}

void expect24BitDibPixel(const std::string &dib, int32_t x, int32_t y, uint8_t red, uint8_t green, uint8_t blue)
{
  const auto width = readS32LE(dib.data() + 4);
  const auto height = readS32LE(dib.data() + 8);
  QVERIFY(width > 0);
  QVERIFY(height > 0);
  QVERIFY(x >= 0 && x < width);
  QVERIFY(y >= 0 && y < height);

  const auto rowStride = static_cast<size_t>((width * 24 + 31) / 32) * 4;
  const auto row = static_cast<size_t>(height - 1 - y);
  const auto offset = kBitmapInfoHeaderSize + row * rowStride + static_cast<size_t>(x) * 3;
  QVERIFY(dib.size() >= offset + 3);
  QCOMPARE(static_cast<uint8_t>(dib[offset + 0]), blue);
  QCOMPARE(static_cast<uint8_t>(dib[offset + 1]), green);
  QCOMPARE(static_cast<uint8_t>(dib[offset + 2]), red);
}

void expectOnePixel24BitDib(const std::string &dib, uint8_t red, uint8_t green, uint8_t blue)
{
  QVERIFY(dib.size() >= kBitmapInfoHeaderSize + 4);
  QCOMPARE(readU32LE(dib.data()), static_cast<uint32_t>(kBitmapInfoHeaderSize));
  QCOMPARE(readS32LE(dib.data() + 4), static_cast<int32_t>(1));
  QCOMPARE(readS32LE(dib.data() + 8), static_cast<int32_t>(1));
  QCOMPARE(readU16LE(dib.data() + 12), static_cast<uint16_t>(1));
  QCOMPARE(readU16LE(dib.data() + 14), static_cast<uint16_t>(24));
  QCOMPARE(readU32LE(dib.data() + 16), static_cast<uint32_t>(0));
  expect24BitDibPixel(dib, 0, 0, red, green, blue);
}

} // namespace

void OSXClipboardTests::open()
{
  OSXClipboard clipboard;
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.close();
}

void OSXClipboardTests::singleFormat()
{
  using enum IClipboard::Format;

  OSXClipboard clipboard;
  QVERIFY(clipboard.empty());
  clipboard.add(Text, m_testString);
  QVERIFY(clipboard.has(Text));
  QCOMPARE(clipboard.get(Text), m_testString);
}

void OSXClipboardTests::formatConvert_UTF8()
{
  OSXClipboardUTF8Converter converter;
  QCOMPARE(IClipboard::Format::Text, converter.getFormat());
  QCOMPARE(converter.getOSXFormat(), CFSTR("public.utf8-plain-text"));
  QCOMPARE(converter.fromIClipboard("test data\n"), "test data\r");
  QCOMPARE(converter.toIClipboard("test data\r"), "test data\n");
}

void OSXClipboardTests::formatConvert_PNG()
{
  OSXClipboardImageConverter converter(CFSTR("public.png"), CFSTR("public.png"));
  QCOMPARE(IClipboard::Format::Bitmap, converter.getFormat());
  QCOMPARE(converter.getOSXFormat(), CFSTR("public.png"));

  const auto image = converter.fromIClipboard(make24BitDib(0x11, 0x7f, 0xe3));
  QVERIFY(!image.empty());

  const auto dib = converter.toIClipboard(image);
  expectOnePixel24BitDib(dib, 0x11, 0x7f, 0xe3);
}

void OSXClipboardTests::formatConvert_TIFF()
{
  OSXClipboardImageConverter converter(CFSTR("public.tiff"), CFSTR("public.tiff"));
  QCOMPARE(IClipboard::Format::Bitmap, converter.getFormat());
  QCOMPARE(converter.getOSXFormat(), CFSTR("public.tiff"));

  const auto image = converter.fromIClipboard(make24BitDib(0x7a, 0x22, 0x05));
  QVERIFY(!image.empty());

  const auto dib = converter.toIClipboard(image);
  expectOnePixel24BitDib(dib, 0x7a, 0x22, 0x05);
}

void OSXClipboardTests::formatConvert_PNG_zeroAlphaBitmap()
{
  OSXClipboardImageConverter converter(CFSTR("public.png"), CFSTR("public.png"));

  const auto image = converter.fromIClipboard(make32BitDib(0xd0, 0x41, 0x19, 0x00));
  QVERIFY(!image.empty());

  const auto dib = converter.toIClipboard(image);
  expectOnePixel24BitDib(dib, 0xd0, 0x41, 0x19);
}

void OSXClipboardTests::formatConvert_PNG_preservesOrientation()
{
  OSXClipboardImageConverter converter(CFSTR("public.png"), CFSTR("public.png"));

  const auto image = converter.fromIClipboard(make24BitDib2x2());
  QVERIFY(!image.empty());

  const auto dib = converter.toIClipboard(image);
  QCOMPARE(readS32LE(dib.data() + 4), static_cast<int32_t>(2));
  QCOMPARE(readS32LE(dib.data() + 8), static_cast<int32_t>(2));
  expect24BitDibPixel(dib, 0, 0, 0xff, 0x00, 0x00);
  expect24BitDibPixel(dib, 1, 0, 0x00, 0xff, 0x00);
  expect24BitDibPixel(dib, 0, 1, 0x00, 0x00, 0xff);
  expect24BitDibPixel(dib, 1, 1, 0xff, 0xff, 0xff);
}

QTEST_MAIN(OSXClipboardTests)

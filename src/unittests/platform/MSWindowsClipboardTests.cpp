/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsClipboardTests.h"

#include "platform/MSWindowsClipboard.h"
#include "platform/MSWindowsClipboardBitmapConverter.h"

#include <QScopeGuard>
#include <QtEndian>

void MSWindowsClipboardTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);

  MSWindowsClipboard clipboard(NULL);

  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
}

void MSWindowsClipboardTests::cleanupTestCase()
{
  initTestCase();
}

void MSWindowsClipboardTests::emptyUnusedClipboard()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.emptyUnowned());
}

void MSWindowsClipboardTests::emptyOpenCalled()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
}

void MSWindowsClipboardTests::emptySingleFormat()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));

  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.empty());
  QVERIFY(!clipboard.has(IClipboard::Format::Text));
}

void MSWindowsClipboardTests::addValue()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));

  clipboard.add(IClipboard::Format::Text, m_testString);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);
}

void MSWindowsClipboardTests::replaceValue()
{
  using enum IClipboard::Format;

  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));

  clipboard.add(Text, m_testString);
  clipboard.add(Text, m_testString2);

  QCOMPARE(clipboard.get(Text), m_testString2);
}

void MSWindowsClipboardTests::openTimeIsOne()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
}

void MSWindowsClipboardTests::closeIsOpen()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  clipboard.close();
}

void MSWindowsClipboardTests::getTimeOpenWithNoEmpty()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  // this behavior is different to that of Clipboard which only
  // returns the value passed into open(t) after empty() is called.
  QCOMPARE(clipboard.getTime(), 1);
}

void MSWindowsClipboardTests::getTimeOpenAndEmpty()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(1));
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.getTime(), 1);
}

void MSWindowsClipboardTests::has_withFormatAdded()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.has(IClipboard::Format::Text));
}

void MSWindowsClipboardTests::has_withNoFormatAdded()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.get(IClipboard::Format::Text), "");
}

void MSWindowsClipboardTests::getNonEmptyText()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  clipboard.add(IClipboard::Format::Text, m_testString);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);
}

void MSWindowsClipboardTests::isOwnedByDeskflow()
{
  MSWindowsClipboard clipboard(NULL);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.isOwnedByDeskflow());
}

void MSWindowsClipboardTests::normalisesMalformedMacBitmap()
{
  // A 1x1 top-down macOS DIB that incorrectly declares a V5 header.
  constexpr qsizetype headerSize = sizeof(BITMAPINFOHEADER);
  std::string dib(headerSize + 4, '\0');
  auto *raw = reinterpret_cast<quint8 *>(&dib[0]);
  qToLittleEndian<quint32>(sizeof(BITMAPV5HEADER), raw); // claims V5 but only has an INFOHEADER
  qToLittleEndian<quint32>(1, raw + 4);
  qToLittleEndian<quint32>(-1, raw + 8);
  qToLittleEndian<quint16>(1, raw + 12);
  qToLittleEndian<quint16>(32, raw + 14);
  qToLittleEndian<quint32>(BI_BITFIELDS, raw + 16);
  raw[headerSize + 3] = 0xff;

  MSWindowsClipboardBitmapConverter converter;
  const auto handle = converter.fromIClipboard(dib);
  QVERIFY(handle != nullptr);
  QCOMPARE(GlobalSize(handle), SIZE_T(headerSize + 4));
  const auto *result = static_cast<const quint8 *>(GlobalLock(handle));
  QVERIFY(result != nullptr);
  QCOMPARE(qFromLittleEndian<quint32>(result), quint32(headerSize));
  QCOMPARE(qFromLittleEndian<quint32>(result + 16), quint32(BI_RGB));
  QCOMPARE(result[headerSize + 3], quint8(0xff));
  GlobalUnlock(handle);
  GlobalFree(handle);
}

void MSWindowsClipboardTests::preservesHealthyMacV5Bitmap()
{
  // A complete 1x1 top-down macOS V5 DIB with BGRA colour masks and one pixel.
  constexpr qsizetype headerSize = sizeof(BITMAPV5HEADER);
  std::string dib(headerSize + 4, '\0');
  auto *raw = reinterpret_cast<quint8 *>(&dib[0]);
  qToLittleEndian<quint32>(headerSize, raw);
  qToLittleEndian<quint32>(1, raw + 4);
  qToLittleEndian<quint32>(-1, raw + 8);
  qToLittleEndian<quint16>(1, raw + 12);
  qToLittleEndian<quint16>(32, raw + 14);
  qToLittleEndian<quint32>(BI_BITFIELDS, raw + 16);
  qToLittleEndian<quint32>(0x00ff0000, raw + 40);
  qToLittleEndian<quint32>(0x0000ff00, raw + 44);
  qToLittleEndian<quint32>(0x000000ff, raw + 48);
  qToLittleEndian<quint32>(0xff000000, raw + 52);
  raw[headerSize] = 0x12;
  raw[headerSize + 1] = 0x34;
  raw[headerSize + 2] = 0x56;
  raw[headerSize + 3] = 0x78;

  MSWindowsClipboardBitmapConverter converter;
  const auto handle = converter.fromIClipboard(dib);
  QVERIFY(handle != nullptr);
  QCOMPARE(GlobalSize(handle), SIZE_T(dib.size()));
  const auto *result = static_cast<const char *>(GlobalLock(handle));
  QVERIFY(result != nullptr);
  QCOMPARE(std::string(result, GlobalSize(handle)), dib);
  GlobalUnlock(handle);
  GlobalFree(handle);
}

namespace {
std::string makeMacSrgbV5Bitmap(qint32 height)
{
  constexpr qsizetype headerSize = sizeof(BITMAPV5HEADER);
  std::string dib(headerSize + 16, '\0');
  auto *raw = reinterpret_cast<quint8 *>(dib.data());
  qToLittleEndian<quint32>(headerSize, raw);
  qToLittleEndian<qint32>(2, raw + 4);
  qToLittleEndian<qint32>(height, raw + 8);
  qToLittleEndian<quint16>(1, raw + 12);
  qToLittleEndian<quint16>(32, raw + 14);
  qToLittleEndian<quint32>(BI_BITFIELDS, raw + 16);
  qToLittleEndian<quint32>(16, raw + 20);
  qToLittleEndian<quint32>(0x00ff0000, raw + 40);
  qToLittleEndian<quint32>(0x0000ff00, raw + 44);
  qToLittleEndian<quint32>(0x000000ff, raw + 48);
  qToLittleEndian<quint32>(0xff000000, raw + 52);
  qToLittleEndian<quint32>(LCS_sRGB, raw + 56);
  for (qsizetype i = 0; i < 16; ++i) {
    raw[headerSize + i] = static_cast<quint8>(0x10 + i);
  }
  return dib;
}
} // namespace

void MSWindowsClipboardTests::normalisesMacSrgbV5Bitmap_data()
{
  QTest::addColumn<qint32>("height");
  QTest::newRow("top-down") << qint32(-2);
  QTest::newRow("bottom-up") << qint32(2);
}

void MSWindowsClipboardTests::normalisesMacSrgbV5Bitmap()
{
  QFETCH(qint32, height);
  const auto dib = makeMacSrgbV5Bitmap(height);
  MSWindowsClipboardBitmapConverter converter;
  const auto handle = converter.fromIClipboard(dib);
  QVERIFY(handle != nullptr);
  const auto *result = static_cast<const char *>(GlobalLock(handle));
  if (result == nullptr) {
    GlobalFree(handle);
    QFAIL("Could not lock converted bitmap");
  }
  const std::string actual(result, GlobalSize(handle));
  GlobalUnlock(handle);
  GlobalFree(handle);

  auto expected = dib.substr(0, sizeof(BITMAPINFOHEADER));
  auto *raw = reinterpret_cast<quint8 *>(expected.data());
  qToLittleEndian<quint32>(sizeof(BITMAPINFOHEADER), raw);
  qToLittleEndian<quint32>(BI_RGB, raw + 16);
  qToLittleEndian<quint32>(2, raw + 8);
  const auto pixels = dib.substr(sizeof(BITMAPV5HEADER));
  expected += height < 0 ? pixels.substr(8) + pixels.substr(0, 8) : pixels;
  QCOMPARE(actual, expected);

  const auto owner =
      CreateWindowExW(0, L"STATIC", L"Deskflow clipboard test", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
  QVERIFY(owner != nullptr);
  const auto cleanup = qScopeGuard([&] {
    CloseClipboard();
    DestroyWindow(owner);
  });
  MSWindowsClipboard clipboard(owner);
  clipboard.close();
  QVERIFY(clipboard.open(0));
  const bool emptied = clipboard.empty();
  clipboard.add(IClipboard::Format::Bitmap, dib);
  clipboard.close();
  QVERIFY(emptied);
  QVERIFY(clipboard.open(0));
  const auto bitmap = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = 2;
  info.bmiHeader.biHeight = -2;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;
  char decoded[16]{};
  const auto dc = GetDC(nullptr);
  const int rows = bitmap != nullptr && dc != nullptr ? GetDIBits(dc, bitmap, 0, 2, decoded, &info, DIB_RGB_COLORS) : 0;
  if (dc != nullptr) {
    ReleaseDC(nullptr, dc);
  }
  clipboard.close();
  QVERIFY(bitmap != nullptr);
  QCOMPARE(rows, 2);
  const auto topDown = height < 0 ? pixels : pixels.substr(8) + pixels.substr(0, 8);
  for (size_t pixel = 0; pixel < 4; ++pixel) {
    QCOMPARE(std::string(decoded + pixel * 4, 3), topDown.substr(pixel * 4, 3));
  }
}

void MSWindowsClipboardTests::preservesUnsupportedV5Bitmap_data()
{
  QTest::addColumn<QByteArray>("dib");
  const auto original = makeMacSrgbV5Bitmap(-2);
  const auto addVariant = [&](const char *name, qsizetype offset, quint32 value) {
    auto dib = original;
    qToLittleEndian<quint32>(value, reinterpret_cast<quint8 *>(dib.data()) + offset);
    QTest::newRow(name) << QByteArray(dib.data(), dib.size());
  };
  addVariant("different-red-mask", 40, 0x000000ff);
  addVariant("different-alpha-mask", 52, 0);
  addVariant("calibrated-colour-space", 56, LCS_CALIBRATED_RGB);
  addVariant("colour-table", 32, 1);
  addVariant("profile-offset", 112, 124);
  addVariant("profile-size", 116, 4);
  addVariant("zero-width", 4, 0);
  addVariant("zero-height", 8, 0);
  addVariant("oversized-width", 4, 0x7fffffff);
  QTest::newRow("truncated-header") << QByteArray(original.data(), sizeof(BITMAPV5HEADER) - 1);
  QTest::newRow("truncated-pixels") << QByteArray(original.data(), original.size() - 1);
}

void MSWindowsClipboardTests::preservesUnsupportedV5Bitmap()
{
  QFETCH(QByteArray, dib);
  MSWindowsClipboardBitmapConverter converter;
  const auto handle = converter.fromIClipboard(std::string(dib.constData(), dib.size()));
  QVERIFY(handle != nullptr);
  const auto *result = static_cast<const char *>(GlobalLock(handle));
  if (result == nullptr) {
    GlobalFree(handle);
    QFAIL("Could not lock bitmap");
  }
  const std::string actual(result, GlobalSize(handle));
  GlobalUnlock(handle);
  GlobalFree(handle);
  QCOMPARE(actual, std::string(dib.constData(), dib.size()));
}
QTEST_MAIN(MSWindowsClipboardTests)

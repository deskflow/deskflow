/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXClipboardTests.h"

#include "platform/OSXClipboard.h"
#include "platform/OSXClipboardImageConverter.h"
#include "platform/OSXClipboardUTF8Converter.h"

#include <cstdlib>

#include <QDataStream>
#include <QtEndian>

static std::string twoPixelDib()
{
  QByteArray dib;
  QDataStream stream(&dib, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream << quint32(40) << qint32(2) << qint32(1) << quint16(1) << quint16(32) << quint32(0) << quint32(8) << qint32(0)
         << qint32(0) << quint32(0) << quint32(0);
  stream << quint32(0xFFFF0000) << quint32(0xFF0000FF);
  return dib.toStdString();
}

static bool pasteboardHasFlavor(CFStringRef flavor)
{
  PasteboardRef pasteboard = nullptr;
  if (PasteboardCreate(kPasteboardClipboard, &pasteboard) != noErr)
    return false;
  PasteboardSynchronize(pasteboard);
  PasteboardItemID item = nullptr;
  PasteboardFlavorFlags flags = 0;
  const bool found = PasteboardGetItemIdentifier(pasteboard, 1, &item) == noErr &&
                     PasteboardGetItemFlavorFlags(pasteboard, item, flavor, &flags) == noErr;
  CFRelease(pasteboard);
  return found;
}

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
  QCOMPARE(converter.fromIClipboard("test data\n"), "test data\n");
  QCOMPARE(converter.toIClipboard("test data\r"), "test data\n");
}

void OSXClipboardTests::formatConvert_png()
{
  OSXClipboardImageConverter converter("public.png");
  QCOMPARE(converter.getFormat(), IClipboard::Format::Bitmap);
  QCOMPARE(CFStringCompare(converter.getOSXFormat(), CFSTR("public.png"), 0), kCFCompareEqualTo);

  const auto png = converter.fromIClipboard(twoPixelDib());
  QVERIFY(png.starts_with("\x89PNG"));

  const auto dib = converter.toIClipboard(png);
  QVERIFY(dib.size() >= 40);
  QCOMPARE(qFromLittleEndian<qint32>(dib.data() + 4), 2);
  QCOMPARE(std::abs(qFromLittleEndian<qint32>(dib.data() + 8)), 1);
}

void OSXClipboardTests::add_bitmap_offeredAsPng()
{
  OSXClipboard clipboard;
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::Bitmap, twoPixelDib());

  QVERIFY(pasteboardHasFlavor(CFSTR("public.png")));
  QVERIFY(pasteboardHasFlavor(CFSTR("com.trolltech.anymime.image--png")));
}

void OSXClipboardTests::add_gif_offeredUnderStandardAndQtTypes()
{
  OSXClipboard clipboard;
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::GIF, std::string("GIF89a\0\x01", 8));

  QVERIFY(pasteboardHasFlavor(CFSTR("com.compuserve.gif")));
  QVERIFY(pasteboardHasFlavor(CFSTR("com.trolltech.anymime.image--gif")));
}

void OSXClipboardTests::add_jpeg_sentAsJpegNotPng()
{
  OSXClipboard clipboard;
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::JPEG, std::string("\xff\xd8\xff\xe0", 4));

  QVERIFY(pasteboardHasFlavor(CFSTR("public.jpeg")));
  QVERIFY(pasteboardHasFlavor(CFSTR("com.trolltech.anymime.image--jpeg")));
  QVERIFY(clipboard.has(IClipboard::Format::JPEG));
  QVERIFY(!clipboard.has(IClipboard::Format::PNG));
  QVERIFY(!clipboard.has(IClipboard::Format::Bitmap));
}

QTEST_MAIN(OSXClipboardTests)

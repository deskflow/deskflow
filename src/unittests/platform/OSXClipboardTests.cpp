/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXClipboardTests.h"

#include "platform/OSXClipboard.h"
#include "platform/OSXClipboardUTF8Converter.h"

#include <ApplicationServices/ApplicationServices.h>
#include <QBuffer>
#include <QImage>

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

void OSXClipboardTests::bitmapOnSecondItem()
{
  QImage image(1, 1, QImage::Format_RGB32);
  image.fill(Qt::red);
  QByteArray pngBytes;
  QBuffer pngBuffer(&pngBytes);
  QVERIFY(pngBuffer.open(QIODevice::WriteOnly));
  QVERIFY(image.save(&pngBuffer, "PNG"));

  PasteboardRef pboard = nullptr;
  QCOMPARE(PasteboardCreate(kPasteboardClipboard, &pboard), noErr);
  QCOMPARE(PasteboardClear(pboard), noErr);

  const auto text = std::string("file:///tmp/image.png");
  auto *textData = CFDataCreate(
      kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(text.data()), static_cast<CFIndex>(text.size())
  );
  QVERIFY(textData != nullptr);
  const auto item1 = reinterpret_cast<PasteboardItemID>(1);
  QCOMPARE(
      PasteboardPutItemFlavor(pboard, item1, CFSTR("public.utf8-plain-text"), textData, kPasteboardFlavorNoFlags), noErr
  );
  CFRelease(textData);

  auto *pngData = CFDataCreate(
      kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(pngBytes.constData()), static_cast<CFIndex>(pngBytes.size())
  );
  QVERIFY(pngData != nullptr);
  const auto item2 = reinterpret_cast<PasteboardItemID>(2);
  QCOMPARE(PasteboardPutItemFlavor(pboard, item2, CFSTR("public.png"), pngData, kPasteboardFlavorNoFlags), noErr);
  CFRelease(pngData);

  PasteboardSynchronize(pboard);

  OSXClipboard clipboard;
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.has(IClipboard::Format::Bitmap));
  QVERIFY(!clipboard.get(IClipboard::Format::Bitmap).empty());
  clipboard.close();

  CFRelease(pboard);
}

QTEST_MAIN(OSXClipboardTests)

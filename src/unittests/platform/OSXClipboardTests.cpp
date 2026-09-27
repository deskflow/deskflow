/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXClipboardTests.h"

#include "platform/OSXClipboard.h"
#include "platform/OSXClipboardUTF8Converter.h"

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

void OSXClipboardTests::add_gif_offeredUnderStandardAndQtTypes()
{
  OSXClipboard clipboard;
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::GIF, std::string("GIF89a\0\x01", 8));

  QVERIFY(pasteboardHasFlavor(CFSTR("com.compuserve.gif")));
  QVERIFY(pasteboardHasFlavor(CFSTR("com.trolltech.anymime.image--gif")));
}

QTEST_MAIN(OSXClipboardTests)

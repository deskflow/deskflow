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

void OSXClipboardTests::hasNewContent_identicalRewrite_isIgnored()
{
  // a clipboard manager rewriting the same content must not count as a copy (#10139)
  const auto write = [](OSXClipboard &clipboard, const std::string &text) {
    QVERIFY(clipboard.open(0));
    clipboard.empty();
    clipboard.add(IClipboard::Format::Text, text);
    clipboard.close();
  };
  OSXClipboard deskflow;
  OSXClipboard otherApp;

  write(otherApp, "AAA\nBBB");
  QVERIFY(deskflow.hasNewContent());

  write(otherApp, "AAA\nBBB");
  QVERIFY(!deskflow.hasNewContent());

  write(otherApp, "CCC");
  QVERIFY(deskflow.hasNewContent());

  // after another computer takes the clipboard, copying the same text again is a real copy
  deskflow.forgetContent();
  write(otherApp, "CCC");
  QVERIFY(deskflow.hasNewContent());
}

QTEST_MAIN(OSXClipboardTests)

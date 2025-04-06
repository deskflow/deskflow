/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClipboardTests.h"

#include "../../lib/deskflow/Clipboard.h"

void ClipboardTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void ClipboardTests::basicFunction()
{
  Clipboard clipboard;

  std::string actual = clipboard.marshall();
  // seems to return "\0\0\0\0" but EXPECT_EQ can't assert this,
  // so instead, just assert that first char is '\0'.
  QCOMPARE((int)actual[0], 0);

  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.getTime(), 0);

  clipboard.close();

  QVERIFY(clipboard.open(1));
  QCOMPARE(clipboard.getTime(), 0);
}

void ClipboardTests::basicText()
{
  Clipboard clipboard;
  QVERIFY(clipboard.open(0));
  QVERIFY(!clipboard.has(Clipboard::kText));
  QCOMPARE(clipboard.get(IClipboard::kText), "");

  clipboard.add(Clipboard::kText, kTestString1);
  QVERIFY(clipboard.has(Clipboard::kText));
  QCOMPARE(clipboard.get(IClipboard::kText), kTestString1);

  std::string actual = clipboard.marshall();
  // string contains other data, but 8th char should be kText.
  QCOMPARE(IClipboard::kText, actual[7]);
  QCOMPARE((int)actual[11], kTestString1.length());

  // // marshall closes the clipboard
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  clipboard.add(Clipboard::kText, kTestString2);
  QCOMPARE(clipboard.get(IClipboard::kText), kTestString2);
  clipboard.close();
}

void ClipboardTests::textSize285()
{
  std::string text;
  text.append("Synergy is Free and Open Source Software that lets you ");
  text.append("easily share your mouse and keyboard between multiple ");
  text.append("computers, where each computer has it's own display. No ");
  text.append("special hardware is required, all you need is a local area ");
  text.append("network. Synergy is supported on Windows, Mac OS X and Linux.");

  Clipboard clipboard;
  clipboard.open(0);
  clipboard.add(IClipboard::kText, text);
  clipboard.close();

  std::string actual = clipboard.marshall();

  // 4 asserts here, but that's ok because we're really just asserting 1
  // thing. the 32-bit size value is split into 4 chars. if the size is 285
  // (29 more than the 8-bit max size), the last char "rolls over" to 29
  // (this is caused by a bit-wise & on 0xff and 8-bit truncation). each
  // char before the last stores a bit-shifted version of the number, each
  // 1 more power than the last, which is done by bit-shifting [0] by 24,
  // [1] by 16, [2] by 8 ([3] is not bit-shifted).
  qInfo() << actual;
  QCOMPARE(actual[8], 0);   // 285 >> 24 = 285 / (256^3) = 0
  QCOMPARE(actual[9], 0);   // 285 >> 16 = 285 / (256^2) = 0
  QCOMPARE(actual[10], 1);  // 285 >> 8 = 285 / (256^1) = 1(.11328125)
  QCOMPARE(actual[11], 29); // 285 - 256 = 29
}

void ClipboardTests::htmlText()
{
  Clipboard clipboard;
  clipboard.open(0);
  clipboard.add(IClipboard::kHTML, kTestString1);
  clipboard.close();

  std::string actual = clipboard.marshall();

  // string contains other data, but 8th char should be kHTML.
  QCOMPARE(IClipboard::kHTML, (int)actual[7]);
}

void ClipboardTests::dualText()
{
  Clipboard clipboard;
  clipboard.open(0);
  clipboard.add(IClipboard::kText, kTestString1);
  clipboard.add(IClipboard::kHTML, kTestString2);
  clipboard.close();

  std::string actual = clipboard.marshall();

  // the number of formats is stored inside the first 4 chars.
  // the writeUInt32 function right-aligns numbers in 4 chars,
  // so if you right align 2, it will be "\0\0\0\2" in a string.
  // we assert that the char at the 4th index is 2 (the number of
  // formats that we've added).
  QCOMPARE((int)actual[3], 2);
}

void ClipboardTests::marshalText()
{
  Clipboard clipboard;
  clipboard.open(0);
  clipboard.add(IClipboard::kText, kTestString1);
  clipboard.close();

  std::string actual = clipboard.marshall();
  // string contains other data, but should end in the string we added.
  QCOMPARE(actual.substr(12), kTestString1);
}

void ClipboardTests::unMarshalText()
{
  Clipboard clipboard;
  std::string data;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)0; // 0 formats added
  clipboard.unmarshall(data, 0);
  clipboard.open(0);

  QVERIFY(!clipboard.has(IClipboard::kText));
  clipboard.close();
}

void ClipboardTests::unMarshalText285()
{
  Clipboard clipboard;

  std::string text;
  text.append("Synergy is Free and Open Source Software that lets you ");
  text.append("easily share your mouse and keyboard between multiple ");
  text.append("computers, where each computer has it's own display. No ");
  text.append("special hardware is required, all you need is a local area ");
  text.append("network. Synergy is supported on Windows, Mac OS X and Linux.");

  std::string data;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)1; // 1 format added
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)IClipboard::kText;
  data += (char)0;  // 285 >> 24 = 285 / (256^3) = 0
  data += (char)0;  // 285 >> 16 = 285 / (256^2) = 0
  data += (char)1;  // 285 >> 8 = 285 / (256^1) = 1(.11328125)
  data += (char)29; // 285 - 256 = 29
  data += text;

  clipboard.unmarshall(data, 0);
  clipboard.open(0);
  QCOMPARE(clipboard.get(IClipboard::kText), text);
  clipboard.close();
}

void ClipboardTests::unMarshalTextAndHtml()
{
  Clipboard clipboard;
  std::string data;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)2; // 2 formats added
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)IClipboard::kText;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)14;
  data += kTestString1;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)IClipboard::kHTML;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)10;
  data += kTestString2;

  clipboard.unmarshall(data, 0);
  clipboard.open(0);
  QCOMPARE(clipboard.get(IClipboard::kText), kTestString1);
  QCOMPARE(clipboard.get(IClipboard::kHTML), kTestString2);
  clipboard.close();
}

void ClipboardTests::equalClipboards()
{
  Clipboard clipboard1;
  clipboard1.open(0);
  clipboard1.add(Clipboard::kText, kTestString1);
  clipboard1.close();

  Clipboard clipboard2;
  Clipboard::copy(&clipboard2, &clipboard1);

  clipboard2.open(0);
  QCOMPARE(clipboard2.get(Clipboard::kText), kTestString1);
  clipboard2.close();
}

QTEST_MAIN(ClipboardTests)

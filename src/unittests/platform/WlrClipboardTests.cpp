/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "WlrClipboardTests.h"
#include "platform/EiClipboard.h"

void WlrClipboardTests::initTestCase()
{
  m_clipboard = std::make_unique<deskflow::WlrClipboard>([]() {});
}

void WlrClipboardTests::cleanupTestCase()
{
  m_clipboard.reset();
}

void WlrClipboardTests::testAvailability()
{
  // Under KWin 6.3 on Wayland, zwlr_data_control_manager_v1 should be available
  QVERIFY(m_clipboard != nullptr);
  QCOMPARE(m_clipboard->isAvailable(), true);
}

void WlrClipboardTests::testOpenAndEmpty()
{
  auto cache = m_clipboard->getCache();
  QVERIFY(cache != nullptr);
  QVERIFY(cache->open(0));
  cache->empty();
  QCOMPARE(cache->has(IClipboard::Format::Text), false);
  cache->close();
}

void WlrClipboardTests::testAddAndGetText()
{
  auto cache = m_clipboard->getCache();
  QVERIFY(cache != nullptr);
  QVERIFY(cache->open(0));
  std::string testPayload = "Deskflow WlrClipboard Test String: Hello Wayland \u2602";
  cache->add(IClipboard::Format::Text, testPayload);
  QCOMPARE(cache->has(IClipboard::Format::Text), true);
  std::string retrieved = cache->get(IClipboard::Format::Text);
  QCOMPARE(retrieved, testPayload);
  cache->close();
}

QTEST_MAIN(WlrClipboardTests)

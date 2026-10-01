/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2024 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyboardLayoutManagerTests.h"

#include "deskflow/KeyboardLayoutManager.h"

void KeyboardLayoutManagerTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void KeyboardLayoutManagerTests::remoteLayouts()
{
  std::string remoteLayouts = "ruenuk";
  deskflow::KeyboardLayoutManager manager({"ru", "en", "uk"});

  manager.setRemoteLayouts(remoteLayouts);
  QCOMPARE(manager.getRemoteLayouts(), (std::vector<std::string>{"ru", "en", "uk"}));

  manager.setRemoteLayouts(std::string());
  QVERIFY(manager.getRemoteLayouts().empty());
}

void KeyboardLayoutManagerTests::remoteLayouts_tooShort_returnsEmpty()
{
  deskflow::KeyboardLayoutManager manager({"ru", "en", "uk"});

  manager.setRemoteLayouts("a");
  QVERIFY(manager.getRemoteLayouts().empty());
}

void KeyboardLayoutManagerTests::remoteLayouts_oddLength_returnsEmpty()
{
  deskflow::KeyboardLayoutManager manager({"ru", "en", "uk"});

  manager.setRemoteLayouts("rue");
  QVERIFY(manager.getRemoteLayouts().empty());
}

void KeyboardLayoutManagerTests::localLayout()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);
  QCOMPARE(manager.getLocalLayouts(), (std::vector<std::string>{"ru", "en", "uk"}));
}

void KeyboardLayoutManagerTests::missedLayout()
{
  std::string remoteLayouts = "ruenuk";
  std::vector<std::string> localLayouts = {"en"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  manager.setRemoteLayouts(remoteLayouts);
  QCOMPARE(manager.getMissedLayouts(), "ru, uk");
}

void KeyboardLayoutManagerTests::layoutInstall()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  QVERIFY(!manager.isLayoutInstalled("us"));
  QVERIFY(manager.isLayoutInstalled("en"));
}

void KeyboardLayoutManagerTests::serializeLocalLayouts()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  QCOMPARE(manager.getSerializedLocalLayouts(), "ruenuk");
}

void KeyboardLayoutManagerTests::normalizeLanguageCode_data()
{
  QTest::addColumn<QString>("tag");
  QTest::addColumn<QString>("expected");

  QTest::newRow("bare") << "en" << "en";
  QTest::newRow("simplified") << "zh-Hans" << "zh";
  QTest::newRow("traditional") << "zh-Hant" << "zh";
  QTest::newRow("script-and-region") << "zh-Hant-TW" << "zh";
  QTest::newRow("region") << "pt-BR" << "pt";
  QTest::newRow("uppercase") << "ZH-Hans" << "zh";
  QTest::newRow("empty") << "" << "";
  QTest::newRow("three-letter") << "yue-Hant" << "";
  QTest::newRow("undefined") << "und" << "";
  QTest::newRow("private-use") << "x-private" << "";
  QTest::newRow("digits") << "12" << "";
}

void KeyboardLayoutManagerTests::normalizeLanguageCode()
{
  QFETCH(QString, tag);
  QFETCH(QString, expected);

  QCOMPARE(deskflow::KeyboardLayoutManager::normalizeLanguageCode(tag.toStdString()), expected.toStdString());
}

void KeyboardLayoutManagerTests::normalizedChineseLayout_isInstalled()
{
  using deskflow::KeyboardLayoutManager;
  KeyboardLayoutManager manager({"en", KeyboardLayoutManager::normalizeLanguageCode("zh-Hans")});
  manager.setRemoteLayouts("zh");

  QVERIFY(manager.isLayoutInstalled("zh"));
  QVERIFY(manager.getMissedLayouts().empty());
  QCOMPARE(manager.getSerializedLocalLayouts(), "enzh");
}

QTEST_MAIN(KeyboardLayoutManagerTests)

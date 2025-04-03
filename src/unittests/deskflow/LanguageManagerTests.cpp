/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LanguageManagerTests.h"

#include "../../lib/deskflow/languages/LanguageManager.h"

void LanguageManagerTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(kDEBUG2);
}

void LanguageManagerTests::remoteLanguages()
{
  std::string remoteLanguages = "ruenuk";
  deskflow::languages::LanguageManager manager({"ru", "en", "uk"});

  manager.setRemoteLanguages(remoteLanguages);
  QCOMPARE(manager.getRemoteLanguages(), (std::vector<std::string>{"ru", "en", "uk"}));

  manager.setRemoteLanguages(std::string());
  QVERIFY(manager.getRemoteLanguages().empty());
}

void LanguageManagerTests::localLanguage()
{
  std::vector<std::string> localLanguages = {"ru", "en", "uk"};
  deskflow::languages::LanguageManager manager(localLanguages);
  QCOMPARE(manager.getLocalLanguages(), (std::vector<std::string>{"ru", "en", "uk"}));
}

void LanguageManagerTests::missedLanguage()
{
  std::string remoteLanguages = "ruenuk";
  std::vector<std::string> localLanguages = {"en"};
  deskflow::languages::LanguageManager manager(localLanguages);

  manager.setRemoteLanguages(remoteLanguages);
  QCOMPARE(manager.getMissedLanguages(), "ru, uk");
}

void LanguageManagerTests::languageInstall()
{
  std::vector<std::string> localLanguages = {"ru", "en", "uk"};
  deskflow::languages::LanguageManager manager(localLanguages);

  QVERIFY(!manager.isLanguageInstalled("us"));
  QVERIFY(manager.isLanguageInstalled("en"));
}

void LanguageManagerTests::serializeLocalLanguages()
{
  std::vector<std::string> localLanguages = {"ru", "en", "uk"};
  deskflow::languages::LanguageManager manager(localLanguages);

  QCOMPARE(manager.getSerializedLocalLanguages(), "ruenuk");
}

QTEST_MAIN(LanguageManagerTests)

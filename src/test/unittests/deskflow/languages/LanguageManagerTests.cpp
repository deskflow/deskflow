/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/languages/LanguageManager.h"

#include <gtest/gtest.h>

TEST(LanguageManager, RemoteLanguagesTest)
{
  std::string remoteLanguages = "ruenuk";
  deskflow::languages::LanguageManager manager({"ru", "en", "uk"});

  manager.setRemoteLanguages(remoteLanguages);
  EXPECT_EQ((std::vector<std::string>{"ru", "en", "uk"}), manager.getRemoteLanguages());

  manager.setRemoteLanguages(std::string());
  EXPECT_TRUE(manager.getRemoteLanguages().empty());
}

TEST(LanguageManager, LocalLanguagesTest)
{
  std::vector<std::string> localLanguages = {"ru", "en", "uk"};
  deskflow::languages::LanguageManager manager(localLanguages);

  EXPECT_EQ((std::vector<std::string>{"ru", "en", "uk"}), manager.getLocalLanguages());
}

TEST(LanguageManager, MissedLanguagesTest)
{
  std::string remoteLanguages = "ruenuk";
  std::vector<std::string> localLanguages = {"en"};
  deskflow::languages::LanguageManager manager(localLanguages);

  manager.setRemoteLanguages(remoteLanguages);
  EXPECT_EQ("ru, uk", manager.getMissedLanguages());
}

TEST(LanguageManager, SerializeLocalLanguagesTest)
{
  std::vector<std::string> localLanguages = {"ru", "en", "uk"};
  deskflow::languages::LanguageManager manager(localLanguages);

  EXPECT_EQ("ruenuk", manager.getSerializedLocalLanguages());
}

TEST(LanguageManager, LanguageInstalledTest)
{
  std::vector<std::string> localLanguages = {"ru", "en", "uk"};
  deskflow::languages::LanguageManager manager(localLanguages);

  EXPECT_FALSE(manager.isLanguageInstalled("us"));
  EXPECT_TRUE(manager.isLanguageInstalled("en"));
}

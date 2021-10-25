/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2021 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synergy/languages/LanguageManager.h"
#include "test/global/gtest.h"


TEST(LanguageManager, RemoteLanguagesTest)
{
    std::string remoteLanguages = "ruenuk";
    synergy::languages::LanguageManager manager({"ru", "en", "uk"});

    manager.setRemoteLanguages(remoteLanguages);
    EXPECT_EQ((std::vector<std::string> {"ru", "en", "uk"}), manager.getRemoteLanguages());

    manager.setRemoteLanguages(String());
    EXPECT_TRUE(manager.getRemoteLanguages().empty());
}

TEST(LanguageManager, LocalLanguagesTest)
{
    std::vector<String> localLanguages = {"ru", "en", "uk"};
    synergy::languages::LanguageManager manager(localLanguages);

    EXPECT_EQ((std::vector<std::string> {"ru", "en", "uk"}), manager.getLocalLanguages());
}

TEST(LanguageManager, MissedLanguagesTest)
{
    String remoteLanguages = "ruenuk";
    std::vector<String> localLanguages = {"en"};
    synergy::languages::LanguageManager manager(localLanguages);

    manager.setRemoteLanguages(remoteLanguages);
    EXPECT_EQ("ru, uk", manager.getMissedLanguages());
}

TEST(LanguageManager, SerializeLocalLanguagesTest)
{
    std::vector<String> localLanguages = {"ru", "en", "uk"};
    synergy::languages::LanguageManager manager(localLanguages);

    EXPECT_EQ("ruenuk", manager.getSerializedLocalLanguages());
}

TEST(LanguageManager, LanguageInstalledTest)
{
    std::vector<String> localLanguages = {"ru", "en", "uk"};
    synergy::languages::LanguageManager manager(localLanguages);

    EXPECT_FALSE(manager.isLanguageInstalled("us"));
    EXPECT_TRUE(manager.isLanguageInstalled("en"));
}

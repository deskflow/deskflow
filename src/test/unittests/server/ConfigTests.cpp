/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "lib/server/Config.h"
#include "test/global/gtest.h"

class OnlySystemFilter: public InputFilter::Condition {
    public:
        Condition*        clone() const override {
            return new OnlySystemFilter();
        }
        String            format() const override {
            return "";
        }

        InputFilter::EFilterStatus    match(const Event& ev) override {
            return ev.getType() == Event::kSystem ? InputFilter::kActivate : InputFilter::kNoMatch;
        }
};

TEST(ServerConfigTests, serverconfig_will_deem_inequal_configs_with_different_map_size)
{
    Config a(nullptr);
    Config b(nullptr);
    a.addScreen("screenA");
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_inequal_configs_with_different_cell_names)
{
    Config a(nullptr);
    Config b(nullptr);
    a.addScreen("screenA");
    b.addScreen("screenB");
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_equal_configs_with_same_cell_names)
{
    Config a(nullptr);
    Config b(nullptr);
    EXPECT_TRUE(a.addScreen("screenA"));
    EXPECT_TRUE(a.addScreen("screenB"));
    EXPECT_TRUE(a.addScreen("screenC"));
    EXPECT_TRUE(a.connect("screenA", EDirection::kBottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
    EXPECT_TRUE(a.connect("screenB", EDirection::kLeft, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
    EXPECT_TRUE(b.addScreen("screenA"));
    EXPECT_TRUE(b.addScreen("screenB"));
    EXPECT_TRUE(b.addScreen("screenC"));
    EXPECT_TRUE(b.connect("screenA", EDirection::kBottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
    EXPECT_TRUE(b.connect("screenB", EDirection::kLeft, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
    a.addOption("screenA", kOptionClipboardSharing, 1);
    b.addOption("screenA", kOptionClipboardSharing, 1);
    a.addOption(std::string(), kOptionClipboardSharing, 1);
    b.addOption(std::string(), kOptionClipboardSharing, 1);
    a.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
    b.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
    a.addAlias("screenA", "aliasA");
    b.addAlias("screenA", "aliasA");
    NetworkAddress addr1("localhost", 8080);
    addr1.resolve();
    NetworkAddress addr2("localhost", 8080);
    addr2.resolve();
    a.setSynergyAddress(addr1);
    b.setSynergyAddress(addr2);

    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_configs_with_same_cell_names_different_options)
{
    Config a(nullptr);
    Config b(nullptr);
    a.addScreen("screenA");
    b.addScreen("screenA");
    a.addOption("screenA", kOptionClipboardSharing, 0);
    b.addOption("screenA", kOptionClipboardSharing, 1);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_configs_with_same_cell_names_different_aliases1)
{
    Config a(nullptr);
    Config b(nullptr);
    a.addScreen("screenA");
    b.addScreen("screenA");
    b.addAlias("screenA", "aliasA");
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_configs_with_same_cell_names_different_aliases2)
{
    Config a(nullptr);
    Config b(nullptr);
    a.addScreen("screenA");
    b.addScreen("screenA");
    a.addAlias("screenA", "aliasA");
    b.addAlias("screenA", "aliasB");
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_configs_with_different_global_options)
{
    Config a(nullptr);
    Config b(nullptr);
    a.addScreen("screenA");
    b.addScreen("screenA");
    a.addOption(std::string(), kOptionClipboardSharing, 0);
    b.addOption(std::string(), kOptionClipboardSharing, 1);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_configs_with_different_filters)
{
    Config a(nullptr);
    Config b(nullptr);
    a.addScreen("screenA");
    b.addScreen("screenA");
    a.getInputFilter()->addFilterRule(InputFilter::Rule{new OnlySystemFilter()});
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_configs_with_different_address)
{
    Config a(nullptr);
    Config b(nullptr);
    a.addScreen("screenA");
    b.addScreen("screenA");
    a.setSynergyAddress(NetworkAddress(8080));
    b.setSynergyAddress(NetworkAddress(1010));
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_cell_neighbours1)
{
    Config a(nullptr);
    Config b(nullptr);
    EXPECT_TRUE(a.addScreen("screenA"));
    EXPECT_TRUE(a.addScreen("screenB"));
    EXPECT_TRUE(a.connect("screenA", EDirection::kBottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
    EXPECT_TRUE(b.addScreen("screenA"));
    EXPECT_TRUE(b.addScreen("screenB"));
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_cell_neighbours2)
{
    Config a(nullptr);
    Config b(nullptr);
    EXPECT_TRUE(a.addScreen("screenA"));
    EXPECT_TRUE(a.addScreen("screenB"));
    EXPECT_TRUE(a.connect("screenA", EDirection::kBottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
    EXPECT_TRUE(b.addScreen("screenA"));
    EXPECT_TRUE(b.addScreen("screenB"));
    EXPECT_TRUE(b.connect("screenA", EDirection::kBottom, 0.0f, 0.25f, "screenB", 0.25f, 1.0f));
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

TEST(ServerConfigTests, serverconfig_will_deem_different_cell_neighbours3)
{
    Config a(nullptr);
    Config b(nullptr);
    EXPECT_TRUE(a.addScreen("screenA"));
    EXPECT_TRUE(a.addScreen("screenB"));
    EXPECT_TRUE(a.addScreen("screenC"));
    EXPECT_TRUE(a.connect("screenA", EDirection::kBottom, 0.0f, 0.5f, "screenB", 0.5f, 1.0f));
    EXPECT_TRUE(b.addScreen("screenA"));
    EXPECT_TRUE(b.addScreen("screenB"));
    EXPECT_TRUE(b.addScreen("screenC"));
    EXPECT_TRUE(b.connect("screenA", EDirection::kBottom, 0.0f, 0.5f, "screenC", 0.5f, 1.0f));
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
}

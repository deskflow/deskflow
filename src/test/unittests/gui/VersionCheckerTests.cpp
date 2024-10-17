/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

#include "gui/VersionChecker.h"
#include "shared/gui/TestQtCoreApp.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace deskflow::gui::proxy;
using namespace testing;

class VersionCheckerTests : public ::testing::Test
{
protected:
  int compareVersions(const QString &left, const QString &right)
  {
    return VersionChecker::compareVersions(left, right);
  }
};

class MockNetworkAccessManager : public QNetworkAccessManagerProxy
{
public:
  MOCK_METHOD(void, init, (), (override));
  MOCK_METHOD(void, get, (const QNetworkRequest &request), (const, override));
};

TEST_F(VersionCheckerTests, checkLatest_callsNetworkGet)
{
  TestQtCoreApp app;
  const auto network = std::make_shared<NiceMock<MockNetworkAccessManager>>();
  const VersionChecker checker(network);

  EXPECT_CALL(*network, get(testing::_)).Times(1);

  checker.checkLatest();
}

TEST_F(VersionCheckerTests, compareVersions_major_isValid)
{
  EXPECT_EQ(compareVersions("1.0.0", "2.0.0"), 1);
  EXPECT_EQ(compareVersions("2.0.0", "1.0.0"), -1);
  EXPECT_EQ(compareVersions("1.0.0", "1.0.0"), 0);
}

TEST_F(VersionCheckerTests, compareVersions_minor_isValid)
{
  EXPECT_EQ(compareVersions("1.1.0", "1.2.0"), 1);
  EXPECT_EQ(compareVersions("1.2.0", "1.1.0"), -1);
  EXPECT_EQ(compareVersions("1.1.0", "1.1.0"), 0);
}

TEST_F(VersionCheckerTests, compareVersions_patch_isValid)
{
  EXPECT_EQ(compareVersions("1.0.1", "1.0.2"), 1);
  EXPECT_EQ(compareVersions("1.0.2", "1.0.1"), -1);
  EXPECT_EQ(compareVersions("1.0.1", "1.0.1"), 0);
}

/*
 * Deskflow -- mouse and keyboard sharing utility
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

#define TEST_ENV

#include "deskflow/ArgParser.h"
#include "deskflow/ServerApp.h"
#include "deskflow/ServerArgs.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::NiceMock;

class MockServerApp : public ServerApp
{
public:
  MockServerApp() : ServerApp(nullptr, nullptr)
  {
  }
};

TEST(ServerAppTests, runInner_will_handle_configuration_lifetime)
{
  NiceMock<MockServerApp> app;

  EXPECT_FALSE(app.args().m_config);

  const char *argv[]{"deskflow-server"};
  app.runInner(1, const_cast<char **>(argv), nullptr, [](int, char **) { return 0; });

  EXPECT_TRUE(app.args().m_config);
}

TEST(ServerAppTests, version_printsYear)
{
  NiceMock<MockServerApp> app;
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  app.version();

  std::cout.rdbuf(old);

#ifdef WIN32
  // regex is god awful on windows, so just check that there is a copyright
  EXPECT_THAT(buffer.str(), testing::HasSubstr("Symless Ltd."));
#else
  std::string expectedPattern = ".*Copyright \\(C\\) [0-9]{4}-[0-9]{4} Symless Ltd.*";
  EXPECT_THAT(buffer.str(), testing::MatchesRegex(expectedPattern));
#endif // WIN32
}

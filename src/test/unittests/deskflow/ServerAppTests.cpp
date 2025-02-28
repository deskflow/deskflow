/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
  MockServerApp() : ServerApp(nullptr)
  {
  }
};

TEST(ServerAppTests, runInner_will_handle_configuration_lifetime)
{
  NiceMock<MockServerApp> app;

  EXPECT_FALSE(app.args().m_config);

  const char *argv[]{"deskflow-server"};
  app.runInner(1, const_cast<char **>(argv), [](int, char **) { return 0; });

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
  EXPECT_THAT(buffer.str(), testing::HasSubstr("Deskflow Devs"));
#else
  std::string expectedPattern = ".*[0-9]{4}-[0-9]{4} Deskflow Devs.*";
  EXPECT_THAT(buffer.str(), testing::MatchesRegex(expectedPattern));
#endif // WIN32
}

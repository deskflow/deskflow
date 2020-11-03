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

#include "synergy/ArgParser.h"
#include "synergy/ServerArgs.h"

#define TEST_ENV

#include "synergy/ServerApp.h"

#include "test/global/gmock.h"

class MockServerApp : public ServerApp
{
public:
    MockServerApp() : ServerApp(nullptr, nullptr) { }
};

#include "test/global/gtest.h"

// using ::testing::_;
// using ::testing::Invoke;
using ::testing::NiceMock;

TEST(ServerAppTests, runInner_will_handle_configuration_lifetime)
{
    NiceMock<MockServerApp> app;

    EXPECT_FALSE(app.args().m_config);

    const char *argv[] {"synergyc"};
    app.runInner(1, const_cast<char **>(argv), nullptr, [](int,char**){ return 0; });

    EXPECT_TRUE(app.args().m_config);
}

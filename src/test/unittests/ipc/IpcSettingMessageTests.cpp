/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2022 Symless Ltd.
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

#include "ipc/IpcSettingMessage.h"
#include "test/global/gtest.h"

TEST(IpcSettingMessage, testIpcSettingMessage) {
    const std::string expected_name = "test";
    const std::string expected_value = "test_value";

    IpcSettingMessage message("test", "test_value");

    EXPECT_EQ(expected_name, message.getName());
    EXPECT_EQ(expected_value, message.getValue());
}

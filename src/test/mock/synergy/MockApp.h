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

#pragma once

#define TEST_ENV

#include "synergy/App.h"

#include "test/global/gmock.h"

class MockApp : public App
{
public:
    MockApp() : App(NULL, NULL, NULL) { }

    MOCK_METHOD(void, help, (), (override));
    MOCK_METHOD(void, loadConfig, (), (override));
    MOCK_METHOD(bool, loadConfig, (const String&), (override));
    MOCK_METHOD(const char*, daemonInfo, (), (const, override));
    MOCK_METHOD(const char*, daemonName, (), (const, override));
    MOCK_METHOD(void, parseArgs, (int, const char* const*), (override));
    MOCK_METHOD(void, version, (), (override));
    MOCK_METHOD(int, standardStartup, (int, char**), (override));
    MOCK_METHOD(int, runInner, (int, char**, ILogOutputter*, StartupFunc), (override));
    MOCK_METHOD(void, startNode, (), (override));
    MOCK_METHOD(int, mainLoop, (), (override));
    MOCK_METHOD(int, foregroundStartup, (int, char**), (override));
    MOCK_METHOD(synergy::Screen*, createScreen, (), (override));
};

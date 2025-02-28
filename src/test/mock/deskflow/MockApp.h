/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#define TEST_ENV

#include "deskflow/App.h"

#include <gmock/gmock.h>

class MockApp : public App
{
public:
  MockApp() : App(NULL, NULL)
  {
  }

  MOCK_METHOD(void, help, (), (override));
  MOCK_METHOD(void, loadConfig, (), (override));
  MOCK_METHOD(bool, loadConfig, (const std::string &), (override));
  MOCK_METHOD(const char *, daemonInfo, (), (const, override));
  MOCK_METHOD(const char *, daemonName, (), (const, override));
  MOCK_METHOD(void, parseArgs, (int, const char *const *), (override));
  MOCK_METHOD(void, version, (), (override));
  MOCK_METHOD(int, standardStartup, (int, char **), (override));
  MOCK_METHOD(int, runInner, (int, char **, StartupFunc), (override));
  MOCK_METHOD(void, startNode, (), (override));
  MOCK_METHOD(int, mainLoop, (), (override));
  MOCK_METHOD(int, foregroundStartup, (int, char **), (override));
  MOCK_METHOD(deskflow::Screen *, createScreen, (), (override));
  MOCK_METHOD(std::string, configSection, (), (const, override));
};

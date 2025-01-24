/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#if 0 // TODO: reintroduce main window integ test once moved to the `gui` lib

// TODO: fix test freezing only on windows
#ifndef WIN32

#include "MainWindow.h"

#include "test/shared/gui/QtTest.h"

#include <gtest/gtest.h>
#include <memory>

class MainWindowTests : public QtTest {
public:
  static void SetUpTestSuite() {
    QtTest::SetUpTestSuite();
    qRegisterMetaType<Edition>("Edition");
  }
};

class TestMainWindow {
public:
  class MainWindowProxy : public MainWindow {
  public:
    explicit MainWindowProxy(AppConfig &appConfig) : MainWindow(appConfig) {}

    bool _checkSecureSocket(const char *test) {
      return MainWindow::checkSecureSocket(test);
    }
  };

  TestMainWindow() {
    m_mainWindow = std::make_shared<MainWindowProxy>(m_appConfig);
  }

  AppConfig m_appConfig;
  std::shared_ptr<MainWindowProxy> m_mainWindow;
};

TEST_F(MainWindowTests, checkSecureSocket_noMatch_expectFalse) {
  TestMainWindow testMainWindow;

  bool result = testMainWindow.m_mainWindow->_checkSecureSocket("test");

  EXPECT_FALSE(result);
}

TEST_F(MainWindowTests, checkSecureSocket_match_expectTrue) {
  TestMainWindow testMainWindow;

  const char *test = "network encryption protocol: test";
  bool result = testMainWindow.m_mainWindow->_checkSecureSocket(test);

  EXPECT_TRUE(result);
}

#endif // WIN32

#endif // 0

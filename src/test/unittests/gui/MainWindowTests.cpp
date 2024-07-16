/*
 * synergy -- mouse and keyboard sharing utility
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

#include "MainWindow.h"

#include "test/mock/gui/QtTests.h"

#include <gtest/gtest.h>
#include <memory>

class MainWindowTests : public QtTests {
public:
  static void SetUpTestSuite() {
    QtTests::SetUpTestSuite();
    qRegisterMetaType<Edition>("Edition");
  }

  static std::shared_ptr<QApplication> s_app;
};

class TestMainWindow {
public:
  class MainWindowProxy : public MainWindow {
  public:
#ifdef SYNERGY_ENABLE_LICENSING
    explicit MainWindowProxy(
        AppConfig &appConfig, LicenseManager &licenseManager)
        : MainWindow(appConfig, licenseManager) {}
#else
    explicit MainWindowProxy(AppConfig &appConfig) : MainWindow(appConfig) {}
#endif

    bool _checkSecureSocket(const char *test) {
      return MainWindow::checkSecureSocket(test);
    }
  };

  TestMainWindow() {

    m_appConfig = std::make_shared<AppConfig>(false);

#ifdef SYNERGY_ENABLE_LICENSING
    m_licenseManager = std::make_shared<LicenseManager>(m_appConfig.get());
    m_mainWindow =
        std::make_shared<MainWindowProxy>(*m_appConfig, *m_licenseManager);
#else
    m_mainWindow = std::make_shared<MainWindowProxy>(*m_appConfig);
#endif
  }

  std::shared_ptr<AppConfig> m_appConfig;
  std::shared_ptr<LicenseManager> m_licenseManager;
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

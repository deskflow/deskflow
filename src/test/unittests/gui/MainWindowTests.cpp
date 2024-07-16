#include "MainWindow.h"

#include <gtest/gtest.h>
#include <memory>

class TestMainWindow {
public:
  class MainWindowProxy : public MainWindow {
  public:
#ifdef SYNERGY_ENABLE_LICENSING
    explicit MainWindowProxy(AppConfig &appConfig,
                             LicenseManager &licenseManager)
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

TEST(MainWindowTests, checkSecureSocket_noMatch_expectFalse) {
  TestMainWindow testMainWindow;

  bool result = testMainWindow.m_mainWindow->_checkSecureSocket("test");

  EXPECT_FALSE(result);
}

TEST(MainWindowTests, checkSecureSocket_match_expectTrue) {
  TestMainWindow testMainWindow;

  const char *test = "network encryption protocol: test";
  bool result = testMainWindow.m_mainWindow->_checkSecureSocket(test);

  EXPECT_TRUE(result);
}

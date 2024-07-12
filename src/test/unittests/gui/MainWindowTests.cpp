#define protected public
#include "MainWindow.h"

#include <gtest/gtest.h>

TEST(MainWindowTests, checkSecureSocket_noMatch_expectFalse) {
  AppConfig appConfig;
  MainWindow mainWindow(appConfig);

  bool result = mainWindow.checkSecureSocket("test");

  EXPECT_FALSE(result);
}

TEST(MainWindowTests, checkSecureSocket_match_expectTrue) {
  AppConfig appConfig;
  MainWindow mainWindow(appConfig);

  bool result =
      mainWindow.checkSecureSocket("network encryption protocol: test");

  EXPECT_TRUE(result);
}

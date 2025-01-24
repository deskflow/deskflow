/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/AppUtil.h"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#define ARCH_APP_UTIL AppUtilWindows

class IEventQueue;

enum AppExitMode
{
  kExitModeNormal,
  kExitModeDaemon
};

class AppUtilWindows : public AppUtil
{
public:
  AppUtilWindows(IEventQueue *events);
  virtual ~AppUtilWindows();

  static AppUtilWindows &instance();

  int daemonNTStartup(int, char **);
  int daemonNTMainLoop(int argc, const char **argv);
  void debugServiceWait();
  int run(int argc, char **argv) override;
  void exitApp(int code) override;
  void beforeAppExit() override;
  void startNode() override;
  std::vector<std::string> getKeyboardLayoutList() override;
  std::string getCurrentLanguageCode() override;
  HKL getCurrentKeyboardLayout() const;
  void showNotification(const std::string &title, const std::string &text) const override;

private:
  AppExitMode m_exitMode;
  IEventQueue *m_events;

  static BOOL WINAPI consoleHandler(DWORD Event);
};

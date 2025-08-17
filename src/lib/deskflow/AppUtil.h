/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/DeskflowException.h"
#include "deskflow/IApp.h"

#include <string>
#include <vector>

class AppUtil
{
public:
  AppUtil();
  virtual ~AppUtil() = default;

  void adoptApp(IApp *app);
  IApp &app() const;
  virtual void exitApp(int code)
  {
    throw ExitAppException(code);
  }

  static AppUtil &instance();
  static void exitAppStatic(int code)
  {
    instance().exitApp(code);
  }

  // Virtual Methods subclasses can impliment
  virtual int run(int argc, char **argv) = 0;
  virtual void startNode() = 0;
  virtual std::vector<std::string> getKeyboardLayoutList() = 0;
  virtual std::string getCurrentLanguageCode() = 0;

private:
  IApp *m_app = nullptr;
  static AppUtil *s_instance;
};

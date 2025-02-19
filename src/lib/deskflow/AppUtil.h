/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IAppUtil.h"
#include "deskflow/XDeskflow.h"

class AppUtil : public IAppUtil
{
public:
  AppUtil();
  virtual ~AppUtil();

  virtual void adoptApp(IApp *app);
  IApp &app() const;
  virtual void exitApp(int code)
  {
    throw XExitApp(code);
  }

  static AppUtil &instance();
  static void exitAppStatic(int code)
  {
    instance().exitApp(code);
  }

private:
  IApp *m_app;
  static AppUtil *s_instance;
};

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/AppUtil.h"

#include <assert.h>

AppUtil *AppUtil::s_instance = nullptr;

AppUtil::AppUtil()
{
  s_instance = this;
}

AppUtil::~AppUtil()
{
  // App objects (and their AppUtil member) are constructed and destroyed
  // per role epoch in auto mode; a dangling static here would make
  // instance() return a dead object in the next epoch.
  if (s_instance == this) {
    s_instance = nullptr;
  }
}

void AppUtil::adoptApp(IApp *app)
{
  app->setByeFunc(&exitAppStatic);
  m_app = app;
}

IApp &AppUtil::app() const
{
  assert(m_app != nullptr);
  return *m_app;
}

AppUtil &AppUtil::instance()
{
  assert(s_instance != nullptr);
  return *s_instance;
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/AppUtil.h"

#include "algorithm"

AppUtil *AppUtil::s_instance = nullptr;

AppUtil::AppUtil() : m_app(nullptr)
{
  s_instance = this;
}

AppUtil::~AppUtil()
{
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

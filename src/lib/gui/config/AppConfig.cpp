/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "AppConfig.h"

#include "common/Settings.h"

#include "ConfigScopes.h"

using namespace deskflow::gui;

AppConfig::AppConfig(deskflow::gui::IConfigScopes &scopes, std::shared_ptr<Deps> deps) : m_Scopes(scopes), m_pDeps(deps)
{
  setLoadFromSystemScope(Settings::isSystemScope());
}

void AppConfig::loadScope(ConfigScopes::Scope scope)
{
  using enum ConfigScopes::Scope;

  switch (scope) {
  case User:
    qDebug("loading user settings scope");
    break;

  case System:
    qDebug("loading system settings scope");
    break;

  default:
    qFatal("invalid scope");
  }

  if (m_Scopes.activeScope() == scope) {
    qDebug("already in required scope, skipping");
    return;
  }

  m_Scopes.setActiveScope(scope);

  qDebug("active scope file path: %s", qPrintable(m_Scopes.activeFilePath()));

  // only signal ready if there is at least one setting in the required scope.
  // this prevents the current settings from being set back to default.
  if (m_Scopes.scopeContains("core", m_Scopes.activeScope())) {
    m_Scopes.signalReady();
  } else {
    qDebug("no screen name in scope, skipping");
  }
}

void AppConfig::setLoadFromSystemScope(bool value)
{
  using enum ConfigScopes::Scope;
  if (value) {
    loadScope(System);
  } else {
    loadScope(User);
  }
}

bool AppConfig::isActiveScopeWritable() const
{
  return m_Scopes.isActiveScopeWritable();
}

bool AppConfig::isActiveScopeSystem() const
{
  return m_Scopes.activeScope() == ConfigScopes::Scope::System;
}

///////////////////////////////////////////////////////////////////////////////
// Begin getters
///////////////////////////////////////////////////////////////////////////////

IConfigScopes &AppConfig::scopes() const
{
  return m_Scopes;
}

///////////////////////////////////////////////////////////////////////////////
// End getters
///////////////////////////////////////////////////////////////////////////////

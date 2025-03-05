/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DeskflowSettings.h"

#include <QFile>

DeskflowSettings *DeskflowSettings::instance()
{
  static DeskflowSettings m;
  return &m;
}

DeskflowSettings::DeskflowSettings(QObject *parent) : QObject(parent)
{
  initSettings();
}

void DeskflowSettings::initSettings()
{
  if (!userSettings) {
    userSettings = new QSettings(Settings::UserSettingPath, QSettings::IniFormat);
    qInfo() << "userSettings " << userSettings->fileName();
  }

  if (!systemSettings) {
    systemSettings = new QSettings(Settings::SystemSettingPath, QSettings::IniFormat);
    qInfo() << "systemSettings " << systemSettings->fileName();
  }

  // Only one will be active at anytime, UserScope == 0
  const auto userScopeCheck = !userSettings->value(Settings::Core::Scope).toBool();
  const auto systemScopeCheck = !systemSettings->value(Settings::Core::Scope).toBool();
  if (userScopeCheck && systemScopeCheck)
    m_settings = userSettings;
  else
    m_settings = systemSettings;
}

void DeskflowSettings::cleanSettings()
{
  const QStringList keys = m_settings->allKeys();
  for (const QString &key : keys) {
    if (!validKeys.contains(key))
      m_settings->remove(key);
    if (m_settings->value(key).toString().isEmpty() && !m_settings->value(key).isValid())
      m_settings->remove(key);
  }
}

QVariant DeskflowSettings::defaultValue(const QString &key)
{
  if (key == Settings::Core::Scope) {
    return false;
  }

  if (key == Settings::Gui::LogExpanded) {
    return true;
  }

  return QVariant();
}

bool DeskflowSettings::isWritable()
{
  return instance()->m_settings->isWritable();
}

bool DeskflowSettings::isSystemScope()
{
  return instance()->m_settings->scope() == QSettings::SystemScope;
}

void DeskflowSettings::setScope(bool systemScope)
{
  auto scope = systemScope ? QSettings::SystemScope : QSettings::UserScope;
  if (instance()->m_settings->scope() == scope)
    return;

  const bool wasWritable = instance()->m_settings->isWritable();

  instance()->userSettings->setValue(Settings::Core::Scope, scope);
  instance()->userSettings->sync();

  instance()->systemSettings->setValue(Settings::Core::Scope, scope);
  instance()->systemSettings->sync();

  instance()->initSettings();

  const bool isWritable = instance()->m_settings->isWritable();

  if (isWritable != wasWritable)
    Q_EMIT instance()->writableChanged(isWritable);

  Q_EMIT instance()->scopeChanged(scope == QSettings::SystemScope);
}

void DeskflowSettings::setValue(const QString &key, const QVariant &value)
{
  if (instance()->m_settings->value(key) == value)
    return;

  if (!value.isValid())
    instance()->m_settings->remove(key);
  else if (key == Settings::Core::Scope) {
    instance()->setScope(value.value<QSettings::Scope>());
  } else {
    instance()->m_settings->setValue(key, value);
  }

  instance()->m_settings->sync();
  Q_EMIT instance()->settingsChanged();
}

QVariant DeskflowSettings::value(const QString &key)
{
  return instance()->m_settings->value(key, defaultValue(key));
}

void DeskflowSettings::restoreDefaultSettings()
{
  for (const auto &key : instance()->validKeys) {
    instance()->blockSignals(true);
    instance()->setValue(key, defaultValue(key));
    instance()->blockSignals(false);
    Q_EMIT instance()->settingsChanged();
  }
}

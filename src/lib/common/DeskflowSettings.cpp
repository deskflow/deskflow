/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DeskflowSettings.h"

#include <QFile>
#include <QRect>

DeskflowSettings *DeskflowSettings::instance()
{
  static DeskflowSettings m;
  return &m;
}

void DeskflowSettings::setSettingFile(const QString &settingsFile)
{
  if (instance()->m_portableSettingsFile == settingsFile)
    return;

  instance()->m_portableSettingsFile = settingsFile;
  if (instance()->m_settings)
    instance()->m_settings->deleteLater();
  instance()->m_settings = new QSettings(instance()->m_portableSettingsFile, QSettings::IniFormat);
  qInfo() << "Settings file" << instance()->m_settings->fileName();
}

DeskflowSettings::DeskflowSettings(QObject *parent) : QObject(parent)
{
  m_portableSettingsFile = QStringLiteral("settings.ini");
  if (QFile(m_portableSettingsFile).exists()) {
    m_settings = new QSettings(m_portableSettingsFile, QSettings::IniFormat);
    qInfo() << "Settings file" << m_settings->fileName();
    return;
  }
  initSettings();
}

bool DeskflowSettings::portableSettings()
{
  return (QFile(instance()->m_portableSettingsFile).exists());
}

void DeskflowSettings::initSettings()
{
  if (m_settings)
    m_settings->deleteLater();

  const auto userScopeCheck =
      QSettings(Settings::UserSettingPath, QSettings::IniFormat).value(Settings::Core::Scope).toBool();
  const auto systemScopeCheck =
      QSettings(Settings::SystemSettingPath, QSettings::IniFormat).value(Settings::Core::Scope).toBool();
  const auto systemScope = (userScopeCheck && systemScopeCheck);
  m_settings =
      new QSettings(systemScope ? Settings::SystemSettingPath : Settings::UserSettingPath, QSettings::IniFormat);
  qInfo() << "Settings file" << m_settings->fileName();
}

void DeskflowSettings::cleanSettings()
{
  const QStringList keys = m_settings->allKeys();
  for (const QString &key : keys) {
    if (!Settings::validKeys.contains(key))
      m_settings->remove(key);
    if (m_settings->value(key).toString().isEmpty() && !m_settings->value(key).isValid())
      m_settings->remove(key);
  }
}

QVariant DeskflowSettings::defaultValue(const QString &key)
{
  if ((key == Settings::Core::Scope) || (key == Settings::Gui::Autohide) || (key == Settings::Core::StartedBefore) ||
      (key == Settings::Core::PreventSleep) || (key == Settings::Server::ExternalConfig) ||
      (key == Settings::Client::InvertScrollDirection)) {
    return false;
  }

  if ((key == Settings::Gui::CloseToTray) || (key == Settings::Gui::LogExpanded) ||
      (key == Settings::Gui::SymbolicTrayIcon) || (key == Settings::Gui::CloseReminder) ||
      (key == Settings::Security::TlsEnabled) || (key == Settings::Security::CheckPeers) ||
      (key == Settings::Client::LanguageSync)) {
    return true;
  }

  if (key == Settings::Gui::WindowGeometry)
    return QRect();

  if (key == Settings::Security::KeySize)
    return 2048;

  if (key == Settings::Security::Certificate) {
    const auto baseDir = QFileInfo(instance()->m_settings->fileName()).absolutePath();
    return QStringLiteral("%1/%2/%3").arg(baseDir, kSslDir, kCertificateFilename);
  }

  if (key == Settings::Server::ExternalConfigFile) {
    const auto baseDir = QFileInfo(instance()->m_settings->fileName()).absolutePath();
    return QStringLiteral("%1/%2.conf").arg(baseDir, kAppId);
  }

  if (key == Settings::Client::Binary)
    return kClientBinName;

  if (key == Settings::Server::Binary)
    return kServerBinName;

  return QVariant();
}

bool DeskflowSettings::isWritable()
{
  return instance()->m_settings->isWritable();
}

bool DeskflowSettings::isSystemScope()
{
  if (portableSettings()) {
    return false;
  }
  return instance()->m_settings->scope() == QSettings::SystemScope;
}

void DeskflowSettings::setScope(bool systemScope)
{
  if (portableSettings()) {
    return;
  }

  auto scope = systemScope ? QSettings::SystemScope : QSettings::UserScope;
  if (instance()->m_settings->scope() == scope)
    return;

  const bool wasWritable = instance()->m_settings->isWritable();

  QSettings userSettings(Settings::UserSettingPath, QSettings::IniFormat);
  userSettings.setValue(Settings::Core::Scope, scope);
  userSettings.sync();

  QSettings systemSettings(Settings::SystemSettingPath, QSettings::IniFormat);
  systemSettings.setValue(Settings::Core::Scope, scope);
  systemSettings.sync();

  instance()->initSettings();

  const bool isWritable = instance()->m_settings->isWritable();

  if (isWritable != wasWritable)
    Q_EMIT instance()->writableChanged(isWritable);

  Q_EMIT instance()->scopeChanged(scope == QSettings::SystemScope);
}

const QString DeskflowSettings::settingsFile()
{
  return instance()->m_settings->fileName();
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
  Q_EMIT instance()->settingsChanged(key);
}

QVariant DeskflowSettings::value(const QString &key)
{
  return instance()->m_settings->value(key, defaultValue(key));
}

void DeskflowSettings::restoreDefaultSettings()
{
  for (const auto &key : Settings::validKeys) {
    instance()->setValue(key, defaultValue(key));
  }
}

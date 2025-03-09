/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Settings.h"

#include <QFile>
#include <QRect>

Settings *Settings::instance()
{
  static Settings m;
  return &m;
}

void Settings::setSettingFile(const QString &settingsFile)
{
  if (instance()->m_portableSettingsFile == settingsFile) {
    qDebug().noquote() << "settings file already in use";
    return;
  }

  instance()->m_portableSettingsFile = settingsFile;
  if (instance()->m_settings)
    instance()->m_settings->deleteLater();
  instance()->m_settings = new QSettings(instance()->m_portableSettingsFile, QSettings::IniFormat);
  qInfo().noquote() << "settings file:" << instance()->m_settings->fileName();
}

Settings::Settings(QObject *parent) : QObject(parent)
{
  m_portableSettingsFile = QStringLiteral("settings.ini");
  if (QFile(m_portableSettingsFile).exists()) {
    m_settings = new QSettings(m_portableSettingsFile, QSettings::IniFormat);
    qInfo().noquote() << "settings file:" << m_settings->fileName();
    return;
  }
  initSettings();
}

bool Settings::isPortableSettings()
{
  return (QFile(instance()->m_portableSettingsFile).exists());
}

void Settings::initSettings()
{

  if (m_settings)
    m_settings->deleteLater();

  const auto userScopeCheck = QSettings(UserSettingFile, QSettings::IniFormat).value(Core::Scope).toBool();
  const auto systemScopeCheck = QSettings(SystemSettingFile, QSettings::IniFormat).value(Core::Scope).toBool();
  const auto systemScope = (userScopeCheck && systemScopeCheck);
  m_settings = new QSettings(systemScope ? SystemSettingFile : UserSettingFile, QSettings::IniFormat);
  qInfo().noquote() << "settings file:" << m_settings->fileName();
}

void Settings::cleanSettings()
{
  const QStringList keys = m_settings->allKeys();
  for (const QString &key : keys) {
    if (!m_validKeys.contains(key))
      m_settings->remove(key);
    if (m_settings->value(key).toString().isEmpty() && !m_settings->value(key).isValid())
      m_settings->remove(key);
  }
}

QVariant Settings::defaultValue(const QString &key)
{
  if ((key == Core::Scope) || (key == Gui::Autohide) || (key == Core::StartedBefore) || (key == Core::PreventSleep) ||
      (key == Server::ExternalConfig) || (key == Client::InvertScrollDirection) || (key == Log::ToFile)) {
    return false;
  }

  if ((key == Gui::CloseToTray) || (key == Gui::LogExpanded) || (key == Gui::SymbolicTrayIcon) ||
      (key == Gui::CloseReminder) || (key == Security::TlsEnabled) || (key == Security::CheckPeers) ||
      (key == Client::LanguageSync)) {
    return true;
  }

  if (key == Core::ScreenName)
    return QSysInfo::machineHostName();

  if (key == Gui::WindowGeometry)
    return QRect();

  if (key == Security::Certificate)
    return QStringLiteral("%1/%2/%3").arg(instance()->settingsPath(), kTlsDirName, kTlsCertificateFilename);

  if (key == Security::KeySize)
    return 2048;

  if (key == Log::File)
    return QStringLiteral("%1/%2").arg(QDir::homePath(), kDefaultLogFile);

  if (key == Log::Level)
    return 0;

  if (key == Client::Binary)
    return kClientBinName;

  if (key == Server::Binary)
    return kServerBinName;

  if (key == Core::ElevateMode)
    return Settings::ElevateMode::Always;

  if (key == Server::ExternalConfigFile)
    return QStringLiteral("%1/%2.conf").arg(instance()->settingsPath(), kAppId);

  if (key == Core::Port)
    return 24800;

  if (key == Core::ProcessMode)
    return defaultProcessMode;

  return QVariant();
}

const QString Settings::logLevelText()
{
  return instance()->m_logLevels.at(instance()->value(Log::Level).toInt());
}

bool Settings::isWritable()
{
  return instance()->m_settings->isWritable();
}

bool Settings::isSystemScope()
{
  if (isPortableSettings()) {
    return false;
  }
  return instance()->settingsFile() == SystemSettingFile;
}

void Settings::setScope(bool systemScope)
{
  if (isPortableSettings()) {
    return;
  }

  if (systemScope == isSystemScope())
    return;

  const bool wasWritable = instance()->m_settings->isWritable();

  QSettings userSettings(Settings::UserSettingFile, QSettings::IniFormat);
  userSettings.setValue(Core::Scope, systemScope);
  userSettings.sync();

  QSettings systemSettings(Settings::SystemSettingFile, QSettings::IniFormat);
  systemSettings.setValue(Core::Scope, systemScope);
  systemSettings.sync();

  instance()->initSettings();

  const bool isWritable = instance()->m_settings->isWritable();

  if (isWritable != wasWritable)
    Q_EMIT instance()->writableChanged(isWritable);

  Q_EMIT instance()->scopeChanged(systemScope);
}

const QString Settings::settingsFile()
{
  return instance()->m_settings->fileName();
}

const QString Settings::settingsPath()
{
  return QFileInfo(instance()->m_settings->fileName()).absolutePath();
}

void Settings::setValue(const QString &key, const QVariant &value)
{
  if (instance()->m_settings->value(key) == value)
    return;

  if (!value.isValid())
    instance()->m_settings->remove(key);
  else if (key == Core::Scope) {
    instance()->setScope(value.value<QSettings::Scope>());
  } else {
    instance()->m_settings->setValue(key, value);
  }

  instance()->m_settings->sync();
  Q_EMIT instance()->settingsChanged(key);
}

QVariant Settings::value(const QString &key)
{
  return instance()->m_settings->value(key, defaultValue(key));
}

void Settings::restoreDefaultSettings()
{
  for (const auto &key : m_validKeys) {
    instance()->setValue(key, defaultValue(key));
  }
}

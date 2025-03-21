/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Settings.h"

#include "UrlConstants.h"

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
  instance()->m_settingsProxy->load(instance()->m_portableSettingsFile);
  qInfo().noquote() << "settings file:" << instance()->m_settings->fileName();
}

Settings::Settings(QObject *parent) : QObject(parent)
{
  QString fileToLoad;
  if (QFile(m_portableSettingsFile).exists()) {
    fileToLoad = m_portableSettingsFile;
  } else {
#ifdef Q_OS_WIN
    fileToLoad = SystemSettingFile;
#else
    if (QFile(UserSettingFile).exists())
      fileToLoad = UserSettingFile;
    else if (QFile(SystemSettingFile).exists())
      fileToLoad = SystemSettingFile;
    else
      fileToLoad = UserSettingFile;
#endif
  }

  m_settings = new QSettings(fileToLoad, QSettings::IniFormat);
  m_settingsProxy = std::make_shared<QSettingsProxy>();
  m_settingsProxy->load(fileToLoad);
  qInfo().noquote() << "settings file:" << m_settings->fileName();
}

bool Settings::isPortableSettings()
{
  return (QFile(instance()->m_portableSettingsFile).exists());
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
  if ((key == Gui::Autohide) || (key == Core::StartedBefore) || (key == Core::PreventSleep) ||
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
    return QStringLiteral("%1/%2").arg(instance()->tlsDir(), kTlsCertificateFilename);

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

  if (key == Core::UpdateUrl)
    return kUrlUpdateCheck;

  if (key == Server::ExternalConfigFile)
    return QStringLiteral("%1/%2.conf").arg(instance()->settingsPath(), kAppId);

  if (key == Core::Port)
    return 24800;

  if (key == Core::ProcessMode)
    return defaultProcessMode;

  if (key == Daemon::LogFile) {
    return QStringLiteral("%1/%2").arg(instance()->settingsPath(), kDaemonLogFilename);
  }

  if (key == Daemon::Elevate)
    return true;

  return QVariant();
}

const QString Settings::logLevelText()
{
  return instance()->m_logLevels.at(instance()->value(Log::Level).toInt());
}

QSettingsProxy &Settings::proxy()
{
  return *instance()->m_settingsProxy;
}

void Settings::save(bool emitSaving)
{
  if (emitSaving)
    Q_EMIT instance()->serverSettingsChanged();
  instance()->m_settings->sync();
}

bool Settings::isWritable()
{
  return instance()->m_settings->isWritable();
}

const QString Settings::settingsFile()
{
  return instance()->m_settings->fileName();
}

const QString Settings::settingsPath()
{
  return QFileInfo(instance()->m_settings->fileName()).absolutePath();
}

const QString Settings::tlsDir()
{
  return QStringLiteral("%1/%2").arg(instance()->settingsPath(), kTlsDirName);
}

const QString Settings::tlsLocalDb()
{
  return QStringLiteral("%1/%2").arg(instance()->tlsDir(), kTlsFingerprintLocalFilename);
}

const QString Settings::tlsTrustedServersDb()
{
  return QStringLiteral("%1/%2").arg(instance()->tlsDir(), kTlsFingerprintTrustedServersFilename);
}

const QString Settings::tlsTrustedClientsDb()
{
  return QStringLiteral("%1/%2").arg(instance()->tlsDir(), kTlsFingerprintTrustedClientsFilename);
}

void Settings::setValue(const QString &key, const QVariant &value)
{
  if (instance()->m_settings->value(key) == value)
    return;

  if (!value.isValid())
    instance()->m_settings->remove(key);
  else
    instance()->m_settings->setValue(key, value);

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

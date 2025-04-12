/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Settings.h"

#include "UrlConstants.h"

#include <QCoreApplication>
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
#ifdef Q_OS_WIN
  m_portableSettingsFile = m_portableSettingsFile.arg(QCoreApplication::applicationDirPath(), kAppName);
  if (QFile(m_portableSettingsFile).exists()) {
    fileToLoad = m_portableSettingsFile;
    m_settings = new QSettings(fileToLoad, QSettings::IniFormat);
  } else {
    m_settings = new QSettings(QSettings::NativeFormat, QSettings::UserScope, kAppName, kAppName);
  }
#else
  if (!qEnvironmentVariable("XDG_CONFIG_HOME").isEmpty())
    fileToLoad = QStringLiteral("%1/%2/%2.conf").arg(qEnvironmentVariable("XDG_CONFIG_HOME"), kAppName);
  else if (QFile(UserSettingFile).exists())
    fileToLoad = UserSettingFile;
  else if (QFile(SystemSettingFile).exists())
    fileToLoad = SystemSettingFile;
  else
    fileToLoad = UserSettingFile;
  m_settings = new QSettings(fileToLoad, QSettings::IniFormat);
#endif

  m_settingsProxy = std::make_shared<QSettingsProxy>();
  m_settingsProxy->load(fileToLoad);
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
  if ((key == Gui::Autohide) || (key == Core::StartedBefore) || (key == Core::PreventSleep) ||
      (key == Server::ExternalConfig) || (key == Client::InvertScrollDirection) || (key == Log::ToFile) ||
      (key == Core::StopOnDeskSwitch)) {
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

  if (key == Daemon::Elevate)
    return instance()->isNativeMode();

  if (key == Core::UpdateUrl)
    return kUrlUpdateCheck;

  if (key == Server::ExternalConfigFile)
    return QStringLiteral("%1/%2-server.conf").arg(instance()->settingsPath(), kAppId);

  if (key == Core::Port)
    return 24800;

  if (key == Core::ProcessMode) {
    if (instance()->isNativeMode())
      return Settings::ProcessMode::Service;
    else
      return Settings::ProcessMode::Desktop;
  }

  if (key == Daemon::LogFile) {
#ifdef Q_OS_WIN
    return QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), kDaemonLogFilename);
#else
    return QStringLiteral("%1/%2").arg(instance()->settingsPath(), kDaemonLogFilename);
#endif
  }

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

const QStringList Settings::validKeys()
{
  return instance()->m_validKeys;
}

bool Settings::isWritable()
{
  if (instance()->isNativeMode())
    return true;
  return instance()->m_settings->isWritable();
}

bool Settings::isNativeMode()
{
  return instance()->m_settings->format() == QSettings::NativeFormat;
}

const QString Settings::settingsFile()
{
  return instance()->m_settings->fileName();
}

const QString Settings::settingsPath()
{
  if (instance()->isNativeMode())
    return SystemDir;
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

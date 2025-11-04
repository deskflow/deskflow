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

void Settings::setSettingsFile(const QString &settingsFile)
{
  if (Settings::settingsFile() == settingsFile) {
    qDebug("settings file already set, skipping");
    return;
  }

  if (instance()->m_settings)
    instance()->m_settings->deleteLater();

  instance()->m_settings = new QSettings(settingsFile, QSettings::IniFormat);
  instance()->m_settingsProxy->load(settingsFile);
  qInfo().noquote() << "settings file changed:" << instance()->m_settings->fileName();
}

Settings::Settings(QObject *parent) : QObject(parent)
{
  QString fileToLoad;
#ifdef Q_OS_WIN
  const auto portableFile = portableSettingsFile();
  qDebug().noquote() << "checking for portable settings file at:" << portableFile;
  if (QFile(portableFile).exists())
    fileToLoad = portableFile;
#else
  if (!qEnvironmentVariable("XDG_CONFIG_HOME").isEmpty())
    fileToLoad = QStringLiteral("%1/%2/%2.conf").arg(qEnvironmentVariable("XDG_CONFIG_HOME"), kAppName);
#endif
  else if (QFile(UserSettingFile).exists())
    fileToLoad = UserSettingFile;
  else if (QFile(SystemSettingFile).exists())
    fileToLoad = SystemSettingFile;
  else
    fileToLoad = UserSettingFile;

  m_settings = new QSettings(fileToLoad, QSettings::IniFormat);
  m_settingsProxy = std::make_shared<QSettingsProxy>();
  m_settingsProxy->load(fileToLoad);
  qInfo().noquote() << "initial settings file:" << m_settings->fileName();
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

int Settings::logLevelToInt(const QString &level)
{
  // Can do this better later ?
  if (level.toUpper() == "FATAL") {
    return 0;
  }

  if (level.toUpper() == "ERROR") {
    return 1;
  }

  if (level.toUpper() == "WARNING") {
    return 2;
  }

  if (level.toUpper() == "NOTE") {
    return 3;
  }

  if (level.toUpper() == "INFO") {
    return 4;
  }

  if (level.toUpper() == "DEBUG") {
    return 5;
  }

  if (level.toUpper() == "DEBUG1") {
    return 6;
  }

  if (level.toUpper() == "DEBUG2") {
    return 7;
  }

  return 4; // If all else fail return info
}

QVariant Settings::defaultValue(const QString &key)
{
  if (m_defaultFalseValues.contains(key)) {
    return false;
  }

  if (m_defaultTrueValues.contains(key)) {
    return true;
  }

  if (key == Core::ScreenName)
    return QSysInfo::machineHostName();

  if (key == Gui::WindowGeometry)
    return QRect();

  if (key == Security::Certificate)
    return QStringLiteral("%1/%2").arg(Settings::tlsDir(), kTlsCertificateFilename);

  if (key == Security::KeySize)
    return 2048;

  if (key == Log::File)
    return QStringLiteral("%1/%2").arg(QDir::homePath(), kDefaultLogFile);

  if (key == Log::Level)
    return 4; // INFO

  if (key == Daemon::Elevate)
    return !Settings::isPortableMode();

  if (key == Core::UpdateUrl)
    return kUrlUpdateCheck;

  if (key == Server::ExternalConfigFile)
    return QStringLiteral("%1/%2-server.conf").arg(Settings::settingsPath(), kAppId);

  if (key == Core::Port)
    return 24800;

  if (key == Core::ProcessMode) {
#ifdef Q_OS_WIN
    if (!Settings::isPortableMode())
      return Settings::ProcessMode::Service;
#endif

    return Settings::ProcessMode::Desktop;
  }

  if (key == Daemon::LogFile) {
    return QStringLiteral("%1/%2").arg(Settings::settingsPath(), kDaemonLogFilename);
  }

  if (key == Client::ScrollSpeed) {
    return 120;
  }

  return QVariant();
}

QString Settings::logLevelText()
{
  return Settings::m_logLevels.at(Settings::value(Log::Level).toInt());
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

QStringList Settings::validKeys()
{
  return Settings::m_validKeys;
}

bool Settings::isWritable()
{
  return instance()->m_settings->isWritable();
}

bool Settings::isPortableMode()
{
  // Enable portable mode only if the portable settings file exists in the expected location.
  return QFile(portableSettingsFile()).exists();
}

QString Settings::settingsFile()
{
  return instance()->m_settings->fileName();
}

QString Settings::settingsPath()
{
#ifdef Q_OS_WIN
  if (!isPortableMode())
    return SystemDir;
#endif

  return QFileInfo(instance()->m_settings->fileName()).absolutePath();
}

QString Settings::tlsDir()
{
  return QStringLiteral("%1/%2").arg(instance()->settingsPath(), kTlsDirName);
}

QString Settings::tlsLocalDb()
{
  return QStringLiteral("%1/%2").arg(instance()->tlsDir(), kTlsFingerprintLocalFilename);
}

QString Settings::tlsTrustedServersDb()
{
  return QStringLiteral("%1/%2").arg(instance()->tlsDir(), kTlsFingerprintTrustedServersFilename);
}

QString Settings::tlsTrustedClientsDb()
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

QString Settings::portableSettingsFile()
{
  static const auto filename =
      QStringLiteral("%1/settings/%2.conf").arg(QCoreApplication::applicationDirPath(), kAppName);
  return filename;
}

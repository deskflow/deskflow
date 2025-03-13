/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "QSettingsProxy.h"

#include "common/Settings.h"

/**
 * @brief The base dir for the system settings file.
 *
 * Important: Qt will append the org name as a dir, and the app name as the
 * settings filename, i.e.: `{base-dir}/Deskflow/Deskflow.ini`
 */
QString getSystemSettingsBaseDir()
{
  return Settings::SystemDir;
}

//
// QSettingsProxy
//

void QSettingsProxy::loadUser()
{
  m_pSettings = std::make_unique<QSettings>(Settings::UserSettingFile, QSettings::IniFormat);
}

void QSettingsProxy::loadSystem()
{
  m_pSettings = std::make_unique<QSettings>(Settings::SystemSettingFile, QSettings::IniFormat);
}

int QSettingsProxy::beginReadArray(const QString &prefix)
{
  return m_pSettings->beginReadArray(prefix);
}

void QSettingsProxy::setArrayIndex(int i)
{
  m_pSettings->setArrayIndex(i);
}

QVariant QSettingsProxy::value(const QString &key) const
{
  return m_pSettings->value(key);
}

QVariant QSettingsProxy::value(const QString &key, const QVariant &defaultValue) const
{
  return m_pSettings->value(key, defaultValue);
}

void QSettingsProxy::endArray()
{
  m_pSettings->endArray();
}

void QSettingsProxy::beginWriteArray(const QString &prefix)
{
  m_pSettings->beginWriteArray(prefix);
}

void QSettingsProxy::setValue(const QString &key, const QVariant &value)
{
  m_pSettings->setValue(key, value);
}

void QSettingsProxy::beginGroup(const QString &prefix)
{
  m_pSettings->beginGroup(prefix);
}

void QSettingsProxy::remove(const QString &key)
{
  m_pSettings->remove(key);
}

void QSettingsProxy::endGroup()
{
  m_pSettings->endGroup();
}

bool QSettingsProxy::isWritable() const
{
  return m_pSettings->isWritable();
}

bool QSettingsProxy::contains(const QString &key) const
{
  return m_pSettings->contains(key);
}

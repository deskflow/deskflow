/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QSettings>

namespace deskflow::gui::proxy {

QString getSystemSettingBaseDir();

class QSettingsProxy
{
public:
  virtual ~QSettingsProxy() = default;

  virtual void loadUser();
  virtual void loadSystem();
  virtual void clear()
  {
    m_pSettings->clear();
  }
  virtual void sync()
  {
    m_pSettings->sync();
  }
  virtual int beginReadArray(const QString &prefix);
  virtual void beginWriteArray(const QString &prefix);
  virtual void setArrayIndex(int i);
  virtual QVariant value(const QString &key) const;
  virtual QVariant value(const QString &key, const QVariant &defaultValue) const;
  virtual void endArray();
  virtual void setValue(const QString &key, const QVariant &value);
  virtual void beginGroup(const QString &prefix);
  virtual void endGroup();
  virtual void remove(const QString &key);
  virtual bool isWritable() const;
  virtual bool contains(const QString &key) const;
  virtual QString fileName() const
  {
    return m_pSettings->fileName();
  }

  QSettings &get() const
  {
    return *m_pSettings;
  }

private:
  std::unique_ptr<QSettings> m_pSettings;
};

} // namespace deskflow::gui::proxy

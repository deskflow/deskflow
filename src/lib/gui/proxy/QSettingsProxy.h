/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QSettings>

namespace synergy::gui::proxy {

class QSettingsProxy {
public:
  virtual ~QSettingsProxy() = default;
  virtual int beginReadArray(QAnyStringView prefix);
  virtual void beginWriteArray(QAnyStringView prefix);
  virtual void setArrayIndex(int i);
  virtual QVariant value(QAnyStringView key) const;
  virtual QVariant
  value(QAnyStringView key, const QVariant &defaultValue) const;
  virtual void endArray();
  virtual void setValue(QAnyStringView key, const QVariant &value);
  virtual void beginGroup(QAnyStringView prefix);
  virtual void endGroup();
  virtual void remove(QAnyStringView key);
  virtual bool isWritable() const;
  virtual bool contains(QAnyStringView key) const;

  void set(QSettings &settings) { m_pSettings = &settings; }
  QSettings &get() const { return *m_pSettings; }

private:
  QSettings *m_pSettings;
};

} // namespace synergy::gui::proxy

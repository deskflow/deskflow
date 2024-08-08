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

#include "QSettingsProxy.h"

namespace synergy::gui::proxy {

int QSettingsProxy::beginReadArray(QAnyStringView prefix) {
  return m_pSettings->beginReadArray(prefix);
}

void QSettingsProxy::setArrayIndex(int i) { m_pSettings->setArrayIndex(i); }

QVariant QSettingsProxy::value(QAnyStringView key) const {
  return m_pSettings->value(key);
}

QVariant
QSettingsProxy::value(QAnyStringView key, const QVariant &defaultValue) const {
  return m_pSettings->value(key, defaultValue);
}

void QSettingsProxy::endArray() { m_pSettings->endArray(); }

void QSettingsProxy::beginWriteArray(QAnyStringView prefix) {
  m_pSettings->beginWriteArray(prefix);
}

void QSettingsProxy::setValue(QAnyStringView key, const QVariant &value) {
  m_pSettings->setValue(key, value);
}

void QSettingsProxy::beginGroup(QAnyStringView prefix) {
  m_pSettings->beginGroup(prefix);
}

void QSettingsProxy::remove(QAnyStringView key) { m_pSettings->remove(key); }

void QSettingsProxy::endGroup() { m_pSettings->endGroup(); }

bool QSettingsProxy::isWritable() const { return m_pSettings->isWritable(); }

bool QSettingsProxy::contains(QAnyStringView key) const {
  return m_pSettings->contains(key);
}

} // namespace synergy::gui::proxy

/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "ValidationError.h"

#include "gui/styles.h"

using namespace deskflow::gui;

namespace validators {

void clear(QLabel *label)
{
  if (label) {
    label->setStyleSheet(kStyleErrorInactiveLabel);
    label->setText("");
  }
}

ValidationError::ValidationError(QObject *parent, QLabel *label) : QObject(parent), m_pLabel(label)
{

  if (m_pLabel) {
    clear(m_pLabel);
  }
}

const QString &ValidationError::message() const
{
  return m_message;
}

void ValidationError::setMessage(const QString &message)
{
  m_message = message;

  if (m_pLabel) {
    if (message.isEmpty()) {
      clear(m_pLabel);
    } else {
      m_pLabel->setStyleSheet(kStyleErrorActiveLabel);
      m_pLabel->setText(message);
    }
  }
}

} // namespace validators

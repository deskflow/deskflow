/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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

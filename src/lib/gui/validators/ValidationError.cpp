/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ValidationError.h"

#include "gui/Styles.h"

using namespace deskflow::gui;

namespace validators {

void clear(QLabel *label)
{
  if (label) {
    label->setStyleSheet(kStyleErrorInactiveLabel);
    label->setText("");
  }
}

ValidationError::ValidationError(QObject *parent, QLabel *label) : QObject(parent), m_label(label)
{

  if (m_label) {
    clear(m_label);
  }
}

const QString &ValidationError::message() const
{
  return m_message;
}

void ValidationError::setMessage(const QString &message)
{
  m_message = message;

  if (m_label) {
    if (message.isEmpty()) {
      clear(m_label);
    } else {
      m_label->setStyleSheet(kStyleErrorActiveLabel);
      m_label->setText(message);
    }
  }
}

} // namespace validators

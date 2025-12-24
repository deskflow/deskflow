/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ValidationError.h"

#include "gui/Styles.h"

using namespace deskflow::gui;

namespace validators {

ValidationError::ValidationError(QObject *parent, QLabel *label) : QObject(parent), m_label(label)
{
  clear();
}

const QString &ValidationError::message() const
{
  return m_message;
}

void ValidationError::setMessage(const QString &message)
{
  if (m_message == message)
    return;

  m_message = message;

  if (!m_label)
    return;

  if (message.isEmpty()) {
    clear();
  } else {
    m_label->setStyleSheet(kStyleErrorActiveLabel);
    m_label->setText(message);
  }
}

void ValidationError::clear()
{
  if (!m_label)
    return;
  m_label->setStyleSheet(kStyleErrorInactiveLabel);
  m_label->setText({});
}

} // namespace validators

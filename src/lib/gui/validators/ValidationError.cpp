/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ValidationError.h"

#include <QPalette>

namespace validators {

ValidationError::ValidationError(QObject *parent, QLabel *label) : QObject(parent), m_label(label)
{
  if (!m_label)
    return;

  m_label->clear();
  m_label->setContentsMargins(5, 3, 5, 3);

  auto palette = m_label->palette();
  palette.setColor(QPalette::WindowText, Qt::white);
  palette.setColor(QPalette::Window, QStringLiteral("crimson"));
  m_label->setPalette(palette);
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

  m_label->setAutoFillBackground(!message.isEmpty());
  m_label->setText(message);
}

} // namespace validators

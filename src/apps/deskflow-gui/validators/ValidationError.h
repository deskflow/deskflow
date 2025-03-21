/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QLabel>
#include <QObject>
#include <QString>

namespace validators {

class ValidationError : public QObject
{
  QString m_message;
  QLabel *m_pLabel = nullptr;

public:
  explicit ValidationError(QObject *parent, QLabel *label = nullptr);
  const QString &message() const;
  void setMessage(const QString &message);
};

} // namespace validators

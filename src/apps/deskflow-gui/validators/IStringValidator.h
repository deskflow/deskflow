/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace validators {

class IStringValidator
{
  QString m_Message;

public:
  IStringValidator() = default;
  explicit IStringValidator(const QString &message);
  const QString &getMessage() const;

  virtual bool validate(const QString &input) const = 0;
  virtual ~IStringValidator() = default;
};

} // namespace validators

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IStringValidator.h"

#include <QRegularExpression>

namespace validators {

class ComputerNameValidator : public IStringValidator
{
public:
  explicit ComputerNameValidator(const QString &message);
  bool validate(const QString &input) const override;

private:
  inline static const QRegularExpression m_nameValidator =
      QRegularExpression(QStringLiteral("^[\\w\\._-]{0,255}$"), QRegularExpression::CaseInsensitiveOption);
};

} // namespace validators

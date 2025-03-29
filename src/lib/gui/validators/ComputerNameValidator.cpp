/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerNameValidator.h"

#include <QRegularExpression>

namespace validators {

ComputerNameValidator::ComputerNameValidator(const QString &message) : IStringValidator(message)
{
}

bool ComputerNameValidator::validate(const QString &input) const
{
  auto match = m_nameValidator.match(input);
  return match.hasMatch();
}

} // namespace validators

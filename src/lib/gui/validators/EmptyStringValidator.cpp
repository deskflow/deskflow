/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "EmptyStringValidator.h"

namespace validators {

EmptyStringValidator::EmptyStringValidator(const QString &message) : IStringValidator(message)
{
}

bool EmptyStringValidator::validate(const QString &input) const
{
  return !input.isEmpty();
}

} // namespace validators

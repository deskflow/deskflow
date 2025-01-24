/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SpacesValidator.h"

namespace validators {

SpacesValidator::SpacesValidator(const QString &message) : IStringValidator(message)
{
}

bool SpacesValidator::validate(const QString &input) const
{
  return !input.contains(' ');
}

} // namespace validators

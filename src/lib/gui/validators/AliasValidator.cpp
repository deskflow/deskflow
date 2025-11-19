/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "AliasValidator.h"

#include "ComputerNameValidator.h"
#include "IpAddressValidator.h"
#include "SpacesValidator.h"

#include <QRegularExpression>

namespace validators {

AliasValidator::AliasValidator(QLineEdit *parent, ValidationError *error) : LineEditValidator(parent, error)
{
  addValidator(std::make_unique<SpacesValidator>(tr("Computer name cannot contain spaces")));
  addValidator(std::make_unique<IpAddressValidator>(tr("Aliases may not be ip addresses")));
  addValidator(std::make_unique<ComputerNameValidator>(tr("Contains invalid characters or is too long")));
}

} // namespace validators

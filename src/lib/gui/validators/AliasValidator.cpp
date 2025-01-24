/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/validators/ComputerNameValidator.h"
#include "gui/validators/SpacesValidator.h"

#include "AliasValidator.h"

#include <QRegularExpression>

namespace validators {

AliasValidator::AliasValidator(QLineEdit *parent, ValidationError *error) : LineEditValidator(parent, error)
{
  addValidator(std::make_unique<SpacesValidator>("Computer name cannot contain spaces"));
  addValidator(std::make_unique<ComputerNameValidator>("Contains invalid characters or is too long"));
}

} // namespace validators

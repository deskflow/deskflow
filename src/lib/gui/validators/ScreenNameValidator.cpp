/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenNameValidator.h"

#include "gui/validators/ComputerNameValidator.h"
#include "gui/validators/EmptyStringValidator.h"
#include "gui/validators/ScreenDuplicationsValidator.h"
#include "gui/validators/SpacesValidator.h"
#include "gui/validators/ValidationError.h"

#include <QRegularExpression>
#include <memory>

namespace validators {

ScreenNameValidator::ScreenNameValidator(QLineEdit *lineEdit, ValidationError *error, const ScreenList *pScreens)
    : LineEditValidator(lineEdit, error)
{
  addValidator(std::make_unique<EmptyStringValidator>("Computer name cannot be empty"));
  addValidator(std::make_unique<SpacesValidator>("Computer name cannot contain spaces"));
  addValidator(std::make_unique<ComputerNameValidator>("Contains invalid characters or is too long"));
  addValidator(std::make_unique<ScreenDuplicationsValidator>(
      "A computer with this name already exists", lineEdit ? lineEdit->text() : "", pScreens
  ));
}

} // namespace validators

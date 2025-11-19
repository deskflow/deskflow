/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenNameValidator.h"

#include "ComputerNameValidator.h"
#include "EmptyStringValidator.h"
#include "ScreenDuplicationsValidator.h"
#include "SpacesValidator.h"
#include "ValidationError.h"

#include <QRegularExpression>
#include <memory>

namespace validators {

ScreenNameValidator::ScreenNameValidator(QLineEdit *lineEdit, ValidationError *error, const ScreenList *pScreens)
    : LineEditValidator(lineEdit, error)
{
  addValidator(std::make_unique<EmptyStringValidator>(tr("Computer name cannot be empty")));
  addValidator(std::make_unique<SpacesValidator>(tr("Computer name cannot contain spaces")));
  addValidator(std::make_unique<ComputerNameValidator>(tr("Contains invalid characters or is too long")));
  addValidator(
      std::make_unique<ScreenDuplicationsValidator>(
          tr("A computer with this name already exists"), lineEdit ? lineEdit->text() : "", pScreens
      )
  );
}

} // namespace validators

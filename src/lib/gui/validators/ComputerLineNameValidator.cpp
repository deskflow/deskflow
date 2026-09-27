/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerLineNameValidator.h"

#include "ComputerDuplicationsValidator.h"
#include "ComputerNameValidator.h"
#include "EmptyStringValidator.h"
#include "SpacesValidator.h"
#include "ValidationError.h"

#include "gui/config/ComputerList.h"

#include <QLineEdit>
#include <QRegularExpression>
#include <memory>

namespace validators {

ComputerLineNameValidator::ComputerLineNameValidator(
    QLineEdit *lineEdit, ValidationError *error, const ComputerList *pComputers
)
    : LineEditValidator(lineEdit, error)
{
  addValidator(std::make_unique<EmptyStringValidator>(tr("Computer name cannot be empty")));
  addValidator(std::make_unique<SpacesValidator>(tr("Computer name cannot contain spaces")));
  addValidator(std::make_unique<ComputerNameValidator>(tr("Contains invalid characters or is too long")));
  addValidator(
      std::make_unique<ComputerDuplicationsValidator>(
          tr("A computer with this name already exists"), lineEdit ? lineEdit->text() : "", pComputers
      )
  );
}

} // namespace validators

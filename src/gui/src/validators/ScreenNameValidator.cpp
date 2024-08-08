/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2021 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScreenNameValidator.h"

#include "validators/ComputerNameValidator.h"
#include "validators/EmptyStringValidator.h"
#include "validators/ScreenDuplicationsValidator.h"
#include "validators/SpacesValidator.h"
#include "validators/ValidationError.h"

#include <QRegularExpression>
#include <memory>

namespace validators {

ScreenNameValidator::ScreenNameValidator(
    QLineEdit *lineEdit, ValidationError *error, const ScreenList *pScreens)
    : LineEditValidator(lineEdit, error) {
  addValidator(
      std::make_unique<EmptyStringValidator>("Computer name cannot be empty"));
  addValidator(
      std::make_unique<SpacesValidator>("Computer name cannot contain spaces"));
  addValidator(std::make_unique<ComputerNameValidator>(
      "Contains invalid characters or is too long"));
  addValidator(std::make_unique<ScreenDuplicationsValidator>(
      "A computer with this name already exists",
      lineEdit ? lineEdit->text() : "", pScreens));
}

} // namespace validators

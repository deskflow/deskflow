/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2021 Symless Ltd.
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

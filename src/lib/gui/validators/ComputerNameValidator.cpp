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

#include "ComputerNameValidator.h"

#include <QRegularExpression>

namespace validators {

ComputerNameValidator::ComputerNameValidator(const QString &message)
    : IStringValidator(message) {}

bool ComputerNameValidator::validate(const QString &input) const {
  const QRegularExpression re(
      "^[\\w\\._-]{0,255}$", QRegularExpression::CaseInsensitiveOption);
  auto match = re.match(input);
  auto result = match.hasMatch();
  return result;
}

} // namespace validators

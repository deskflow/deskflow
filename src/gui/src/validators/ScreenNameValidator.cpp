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
#include "validators/EmptyStringValidator.h"
#include "validators/RegExpValidator.h"
#include "validators/ScreenDuplicationsValidator.h"
#include "validators/SpacesValidator.h"

#include <QRegularExpression>
#include <memory>

static const QRegularExpression
    ValidScreenName("[a-z0-9\\._-]{,255}",
                    QRegularExpression::CaseInsensitiveOption);

namespace validators {

ScreenNameValidator::ScreenNameValidator(QLineEdit *parent, QLabel *errors,
                                         const ScreenList *pScreens)
    : LineEditValidator(parent, errors) {
  addValidator(
      std::make_unique<EmptyStringValidator>("Computer name cannot be empty"));
  addValidator(
      std::make_unique<SpacesValidator>("Computer name cannot contain spaces"));
  addValidator(std::make_unique<RegExpValidator>(
      "Computer name contains unsupported characters", ValidScreenName));
  addValidator(std::make_unique<ScreenDuplicationsValidator>(
      "A computer with this name already exists", parent ? parent->text() : "",
      pScreens));
}

} // namespace validators

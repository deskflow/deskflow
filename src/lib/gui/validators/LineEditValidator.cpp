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

#include "LineEditValidator.h"

#include "gui/styles.h"

#include <QValidator>

using namespace deskflow::gui;

namespace validators {

LineEditValidator::LineEditValidator(QLineEdit *lineEdit, ValidationError *error)
    : m_pError(error),
      m_pLineEdit(lineEdit)
{

  if (!m_pLineEdit) {
    qFatal("validator line edit not set");
  }
}

void LineEditValidator::addValidator(std::unique_ptr<IStringValidator> validator)
{
  m_Validators.push_back(std::move(validator));
}

QValidator::State LineEditValidator::validate(QString &input, int &pos) const
{
  assert(m_pLineEdit);

  QString errorMessage;
  for (const auto &validator : m_Validators) {
    if (!validator->validate(input)) {
      errorMessage = validator->getMessage();
      break;
    }
  }

  if (errorMessage.isEmpty()) {
    m_pLineEdit->setStyleSheet("");
  } else {
    m_pLineEdit->setStyleSheet(kStyleLineEditErrorBorder);
  }

  if (m_pError) {
    m_pError->setMessage(errorMessage);
  }

  return errorMessage.isEmpty() ? Acceptable : Intermediate;
}

} // namespace validators

/*
 * synergy -- mouse and keyboard sharing utility
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

#include "gui/constants.h"
#include <qvalidator.h>

namespace validators {

LineEditValidator::LineEditValidator(QLineEdit *lineEdit, QLabel *errorLabel)
    : m_pErrorLabel(errorLabel),
      m_pLineEdit(lineEdit) {

  if (!m_pLineEdit) {
    qFatal("validator line edit not set");
  }

  if (m_pErrorLabel) {
    m_pErrorLabel->hide();
  }
}

void LineEditValidator::addValidator(
    std::unique_ptr<IStringValidator> validator) {
  m_Validators.push_back(std::move(validator));
}

QValidator::State LineEditValidator::validate(QString &input, int &pos) const {
  assert(m_pLineEdit);

  QString error;
  for (const auto &validator : m_Validators) {
    if (!validator->validate(input)) {
      error = validator->getMessage();
      break;
    }
  }

  if (error.isEmpty()) {
    m_pLineEdit->setStyleSheet("");
  } else {
    m_pLineEdit->setStyleSheet(kRedBorder);
  }

  setError(error);
  return error.isEmpty() ? Acceptable : Intermediate;
}

void LineEditValidator::setError(const QString &message) const {
  if (m_pErrorLabel) {
    m_pErrorLabel->setText(message);

    if (m_pErrorLabel->text().isEmpty()) {
      m_pErrorLabel->hide();
    } else {
      m_pErrorLabel->show();
    }
  }
}

} // namespace validators

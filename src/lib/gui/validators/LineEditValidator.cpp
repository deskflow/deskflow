/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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

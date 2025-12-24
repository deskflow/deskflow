/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LineEditValidator.h"

#include <QApplication>
#include <QStyle>
#include <QValidator>

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

QValidator::State LineEditValidator::validate(QString &input, int &) const
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
    m_pLineEdit->setStyleSheet({});
  } else { // The values here are for fusion style
    int radius = 3;
    int padVer = 2;
    int padHor = 0;
    int borderWidth = 1;
    if (const auto wStyle = QApplication::style()->name().toLower(); wStyle == QStringLiteral("breeze")) {
      radius = 6;
      padVer = 5;
      padHor = 5;
    } else if (wStyle == QStringLiteral("macos")) {
      radius = 4;
      padVer = 1;
      borderWidth = 3;
    } else if (wStyle == QStringLiteral("windows")) {
      padVer = 1;
      padHor = 1;
    }
    m_pLineEdit->setStyleSheet(QStringLiteral("border-radius: %1px; padding: %2px %3px; border: %4px solid crimson;")
                                   .arg(
                                       QString::number(radius), QString::number(padVer), QString::number(padHor),
                                       QString::number(borderWidth)
                                   ));
  }

  if (m_pError) {
    m_pError->setMessage(errorMessage);
  }

  return errorMessage.isEmpty() ? Acceptable : Intermediate;
}

} // namespace validators

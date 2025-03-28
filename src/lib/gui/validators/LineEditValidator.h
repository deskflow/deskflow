/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IStringValidator.h"

#include <QLabel>
#include <QLineEdit>
#include <QValidator>
#include <memory>
#include <vector>

#include "ValidationError.h"

namespace validators {

class LineEditValidator : public QValidator
{
public:
  explicit LineEditValidator(QLineEdit *lineEdit = nullptr, ValidationError *error = nullptr);
  QValidator::State validate(QString &input, int &pos) const override;
  void addValidator(std::unique_ptr<IStringValidator> validator);

private:
  ValidationError *m_pError = nullptr;
  QLineEdit *m_pLineEdit = nullptr;
  std::vector<std::unique_ptr<IStringValidator>> m_Validators;

  void setError(const QString &message) const;
};

} // namespace validators

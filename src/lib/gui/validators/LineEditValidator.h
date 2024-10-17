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

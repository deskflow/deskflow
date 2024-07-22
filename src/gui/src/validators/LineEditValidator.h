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
#ifndef LINEEDITVALIDATOR_H
#define LINEEDITVALIDATOR_H

#include <memory>
#include <qtmetamacros.h>
#include <vector>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <qvalidator.h>

#include "IStringValidator.h"

namespace validators {

class LineEditValidator : public QValidator {
  Q_OBJECT

public:
  explicit LineEditValidator(
      QLineEdit *parent = nullptr, QLabel *errors = nullptr);
  QValidator::State validate(QString &input, int &pos) const override;
  void addValidator(std::unique_ptr<IStringValidator> validator);

signals:
  void finished(const QString &message) const;

private:
  QLabel *m_pErrors = nullptr;
  QLineEdit *m_pControl = nullptr;
  std::vector<std::unique_ptr<IStringValidator>> m_Validators;

  void showError(const QString &message) const;
};

} // namespace validators

#endif // LINEEDITVALIDATOR_H

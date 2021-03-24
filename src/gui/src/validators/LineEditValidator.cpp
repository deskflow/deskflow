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
#include "LineEditValidator.h"

namespace validators
{

LineEditValidator::LineEditValidator(QLineEdit* parent, QLabel* errors) :
   m_pErrors(errors),
   m_pControl(parent)
{
   if (m_pErrors) {
      m_pErrors->hide();
   }
}

void LineEditValidator::addValidator(std::unique_ptr<IStringValidator> validator)
{
    m_Validators.push_back(std::move(validator));
}

QValidator::State LineEditValidator::validate(QString& input, int& pos) const
{
   if (m_pControl)
   {
      showError("");
      m_pControl->setStyleSheet("");

      for (const auto& validator : m_Validators)
      {
         if (!validator->validate(input))
         {
            m_pControl->setStyleSheet("border: 1px solid #EC4C47");
            showError(validator->getMessage());
            break;
         }
      }
   }

   return Acceptable;
}

void LineEditValidator::showError(const QString& message) const
{
   if (m_pErrors)
   {
      m_pErrors->setText(message);

      if (m_pErrors->text().isEmpty())
      {
         m_pErrors->hide();
      }
      else
      {
         m_pErrors->show();
      }
   }
}

}

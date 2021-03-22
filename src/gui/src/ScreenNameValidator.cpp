/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

ScreenNameValidator::ScreenNameValidator(QLineEdit* parent, QLabel* errors) :
    QRegExpValidator(QRegExp("[a-z0-9\\._-]{,15}", Qt::CaseInsensitive), parent),
    m_pErrors(errors),
    m_pControl(parent)
{
   if (m_pErrors) {
      m_pErrors->hide();
   }
}

QValidator::State ScreenNameValidator::validate(QString& input, int& pos) const
{
   if (m_pControl) {
      int currentPos = pos;
      if (input.isEmpty() || QRegExpValidator::validate(input, currentPos) == Invalid) {
         m_pControl->setStyleSheet("border: 1px solid #EC4C47");
      }
      else {
         m_pControl->setStyleSheet("");
      }
      showError(getErrorMessage(input));
   }

   return Acceptable;
}

void ScreenNameValidator::showError(const QString& message) const
{
   if (m_pErrors) {
      m_pErrors->setText(message);
      if (m_pErrors->text().isEmpty()) {
         m_pErrors->hide();
      }
      else {
         m_pErrors->show();
      }
   }
}

QString ScreenNameValidator::getErrorMessage(const QString& text) const
{
   QString message;

   if (text.isEmpty()) {
      message = "Computer name can't be empty";
   }
   else
   {
      if (text.contains(' '))
      {
         message = "Remove spaces";
      }
      else
      {
         int pos = 0;
         QString data(text);
         if (QRegExpValidator::validate(data, pos) == Invalid) {
            message = "Remove unsupported characters";
         }
      }
   }

   return message;
}


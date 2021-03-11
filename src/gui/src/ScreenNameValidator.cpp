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

ScreenNameValidator::ScreenNameValidator(QLineEdit* parent) :
    QRegExpValidator(QRegExp("[a-z0-9\\._-]{,15}", Qt::CaseInsensitive), parent),
    m_pControl(parent)
{

}

QValidator::State ScreenNameValidator::validate(QString& input, int& pos) const
{
   isValid(input, pos);
   return  Acceptable;
}

bool ScreenNameValidator::validate() const
{
   bool result = true;

   if (m_pControl) {
      int pos = 0;
      QString text(m_pControl->text());
      result = isValid(text, pos);
   }

   return result;
}

bool ScreenNameValidator::isValid(QString& input, int& pos) const
{
   bool result = true;

   if (m_pControl) {
      if (input.isEmpty() || QRegExpValidator::validate(input, pos) == Invalid) {
         m_pControl->setStyleSheet("border: 1px solid #EC4C47");
         result = false;
      }
      else {
         m_pControl->setStyleSheet("");
      }
   }

   return result;
}

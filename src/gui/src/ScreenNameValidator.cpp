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
#include "Screen.h"
#include "ScreenNameValidator.h"
#include <memory>

namespace {


class EmptyNameValidator : public INameValidator
{
public:
   bool validate(const QString& input) const override
   {
      return !input.isEmpty();
   }

   QString getMessage() const override
   {
      return "Computer name can't be empty";
   }
};

class SpacesValidator : public INameValidator
{
public:
   bool validate(const QString& input) const override
   {
      return !input.contains(' ');
   }

   QString getMessage() const override
   {
      return "Remove spaces";
   }
};

class SpecialCharactersValidator : public INameValidator
{

public:
   bool validate(const QString& input) const override
   {
      auto validator = QRegExp("[a-z0-9\\._-]{,255}", Qt::CaseInsensitive);
      return (validator.exactMatch(input));
   }

   QString getMessage() const override
   {
      return "Remove unsupported characters";
   }
};

class DuplicationsValidator : public INameValidator
{
   const QString m_defaultName;
   const ScreenList* m_pScreenList = nullptr;

public:
   DuplicationsValidator(const QString defaultName,const ScreenList* pScreens) :
      m_defaultName(defaultName),
      m_pScreenList(pScreens)
   {

   }

   bool validate(const QString& input) const override
   {
      bool result = true;

      if (m_pScreenList)
      {
         for(const auto& screen : (*m_pScreenList))
         {
            if (m_defaultName != input && input == screen.name())
            {
               result = false;
               break;
            }
         }
      }

      return result;
   }

   QString getMessage() const override
   {
      return "Computer with this name already exists";
   }
};



}

ScreenNameValidator::ScreenNameValidator(QLineEdit* parent, QLabel* errors, const ScreenList* pScreens) :
    m_pErrors(errors),
    m_pControl(parent)
{
   if (m_pErrors) {
      m_pErrors->hide();
   }

   m_Validators.push_back(std::make_unique<EmptyNameValidator>());
   m_Validators.push_back(std::make_unique<SpacesValidator>());
   m_Validators.push_back(std::make_unique<SpecialCharactersValidator>());
   m_Validators.push_back(std::make_unique<DuplicationsValidator>(m_pControl ? m_pControl->text() : "", pScreens));
}

QValidator::State ScreenNameValidator::validate(QString& input, int& pos) const
{
   if (m_pControl) {
      showError("");
      m_pControl->setStyleSheet("");

      for(const auto& validator : m_Validators)
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

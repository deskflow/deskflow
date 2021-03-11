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
#ifndef SCREENNAMEVALIDATOR_H
#define SCREENNAMEVALIDATOR_H

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <qvalidator.h>

class ScreenNameValidator : public QRegExpValidator
{
public:
   explicit ScreenNameValidator(QLineEdit* parent = nullptr, QLabel* errors = nullptr);
   virtual QValidator::State validate(QString& input, int& pos) const override;

private:
   QLabel* m_pErrors = nullptr;
   QLineEdit* m_pControl = nullptr;

   void showError(const QString& message) const;
   QString getErrorMessage(const QString& text) const;
};

#endif // SCREENNAMEVALIDATOR_H

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
#ifndef IVALIDATOR_H
#define IVALIDATOR_H

#include <QString>

namespace validators
{

class IStringValidator
{
   QString m_Message;

public:
   IStringValidator() = default;
   explicit IStringValidator(const QString& message);
   const QString& getMessage() const;

   virtual bool validate(const QString& input) const = 0;
   virtual ~IStringValidator() = default;
};

}


#endif // IVALIDATOR_H

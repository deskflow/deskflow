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

#include "gui/config/ScreenList.h"

namespace validators {

class ScreenDuplicationsValidator : public IStringValidator
{
  const QString m_defaultName;
  const ScreenList *m_pScreenList = nullptr;

public:
  ScreenDuplicationsValidator(const QString &message, const QString &defaultName, const ScreenList *pScreens);
  bool validate(const QString &input) const override;
};

} // namespace validators

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "LineEditValidator.h"
#include "gui/config/ScreenList.h"
#include "gui/validators/ValidationError.h"

namespace validators {

class ScreenNameValidator : public LineEditValidator
{
public:
  explicit ScreenNameValidator(
      QLineEdit *lineEdit = nullptr, ValidationError *error = nullptr, const ScreenList *pScreens = nullptr
  );
};

} // namespace validators

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "LineEditValidator.h"
#include "ValidationError.h"

#include "gui/config/ScreenList.h"

namespace validators {

class ScreenNameValidator : public LineEditValidator
{
  Q_OBJECT
public:
  explicit ScreenNameValidator(
      QLineEdit *lineEdit = nullptr, ValidationError *error = nullptr, const ScreenList *pScreens = nullptr
  );
};

} // namespace validators

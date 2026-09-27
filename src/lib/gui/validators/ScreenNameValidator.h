/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "LineEditValidator.h"

class ComputerList;

namespace validators {
class ValidationError;

class ScreenNameValidator : public LineEditValidator
{
  Q_OBJECT
public:
  explicit ScreenNameValidator(
      QLineEdit *lineEdit = nullptr, ValidationError *error = nullptr, const ComputerList *pScreens = nullptr
  );
};

} // namespace validators

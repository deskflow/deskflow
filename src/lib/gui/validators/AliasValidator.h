/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "LineEditValidator.h"
#include "gui/validators/ValidationError.h"

namespace validators {

class AliasValidator : public LineEditValidator
{
public:
  explicit AliasValidator(QLineEdit *parent = nullptr, ValidationError *error = nullptr);
};

} // namespace validators

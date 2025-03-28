/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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

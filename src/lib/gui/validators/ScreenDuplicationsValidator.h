/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IStringValidator.h"

class ComputerList;

namespace validators {

class ScreenDuplicationsValidator : public IStringValidator
{
  const QString m_defaultName;
  const ComputerList *m_pComputerList = nullptr;

public:
  ScreenDuplicationsValidator(const QString &message, const QString &defaultName, const ComputerList *pScreens);
  bool validate(const QString &input) const override;
};

} // namespace validators

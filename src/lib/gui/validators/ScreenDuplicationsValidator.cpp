/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenDuplicationsValidator.h"

namespace validators {

ScreenDuplicationsValidator::ScreenDuplicationsValidator(
    const QString &message, const QString &defaultName, const ScreenList *pScreens
)
    : IStringValidator(message),
      m_defaultName(defaultName),
      m_pScreenList(pScreens)
{
}

bool ScreenDuplicationsValidator::validate(const QString &input) const
{
  bool result = true;

  if (m_pScreenList) {
    for (const auto &screen : (*m_pScreenList)) {
      if (!screen.isNull() && !screen.isServer() && input != m_defaultName && input == screen.name()) {
        result = false;
        break;
      }
    }
  }

  return result;
}

} // namespace validators

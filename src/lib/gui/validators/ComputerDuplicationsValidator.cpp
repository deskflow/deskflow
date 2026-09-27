/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerDuplicationsValidator.h"
#include "gui/config/ComputerList.h"

namespace validators {

ComputerDuplicationsValidator::ComputerDuplicationsValidator(
    const QString &message, const QString &defaultName, const ComputerList *pComputers
)
    : IStringValidator(message),
      m_defaultName(defaultName),
      m_pComputerList(pComputers)
{
  // do nothing
}

bool ComputerDuplicationsValidator::validate(const QString &input) const
{
  bool result = true;

  if (m_pComputerList) {
    for (const auto &computer : (*m_pComputerList)) {
      if (!computer.isNull() && !computer.isServer() && input != m_defaultName && input == computer.name()) {
        result = false;
        break;
      }
    }
  }

  return result;
}

} // namespace validators

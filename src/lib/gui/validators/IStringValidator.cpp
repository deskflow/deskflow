/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IStringValidator.h"

namespace validators {

IStringValidator::IStringValidator(const QString &message) : m_Message(message)
{
}

const QString &IStringValidator::getMessage() const
{
  return m_Message;
}

} // namespace validators

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpAddressValidator.h"
#include <QRegularExpression>

namespace validators {

IpAddressValidator::IpAddressValidator(const QString &message) : IStringValidator(message)
{
  // do nothing
}

bool IpAddressValidator::validate(const QString &input) const
{
  static const auto sIpRegex = QRegularExpression(
      R"((\b25[0-5]|\b2[0-4][0-9]|\b[01]?[0-9][0-9]?)(\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)){3})"
  );
  const auto match = sIpRegex.match(input);
  return !match.hasMatch();
}

} // namespace validators

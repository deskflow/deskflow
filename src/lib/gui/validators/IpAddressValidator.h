/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IStringValidator.h"

namespace validators {

class IpAddressValidator : public IStringValidator
{
public:
  explicit IpAddressValidator(const QString &message);
  bool validate(const QString &input) const override;
};

} // namespace validators

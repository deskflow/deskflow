/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ManagedSettings.h"

namespace deskflow::settings::admin {

bool isManaged(const QString &settingsKey)
{
  Q_UNUSED(settingsKey)
  return false;
}

QVariant value(const QString &settingsKey)
{
  Q_UNUSED(settingsKey)
  return QVariant();
}

} // namespace deskflow::settings::admin

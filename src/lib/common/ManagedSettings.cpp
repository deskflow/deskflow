/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Settings.h"

bool Settings::isManaged(const QString &key)
{
  Q_UNUSED(key)
  return false;
}

QVariant Settings::managedValue(const QString &key)
{
  Q_UNUSED(key)
  return QVariant();
}

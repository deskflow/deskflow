/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QVariant>

/**
 * @brief Returns true if an admin policy currently provides a value for the given key.
 * @param settingsKey The settings key to check.
 * @return true if a managed value exists (macOS MDM), false on other platforms.
 */
bool hasManagedValue(const QString &settingsKey);

/**
 * @brief Returns the admin-enforced value for the given key.
 * @param settingsKey The settings key to look up.
 * @return The managed value, or an invalid QVariant if none exists.
 */
QVariant managedSettingValue(const QString &settingsKey);

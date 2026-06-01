/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QVariant>

namespace deskflow::settings::admin {

/**
 * @brief Returns true if the given Settings key is enforced by an admin policy.
 *
 * On macOS, checks NSUserDefaults for MDM-managed preferences.
 * On other platforms, always returns false.
 *
 * @param settingsKey The Settings key string (e.g. Settings::Security::TlsEnabled).
 * @return true if the key is managed by an admin policy, false otherwise.
 */
bool isManaged(const QString &settingsKey);

/**
 * @brief Returns the admin-enforced value for the given Settings key.
 *
 * On macOS, returns the MDM-enforced value if managed.
 * On other platforms, always returns an invalid QVariant.
 *
 * @param settingsKey The Settings key string.
 * @return The enforced value, or an invalid QVariant if not managed.
 */
QVariant value(const QString &settingsKey);

} // namespace deskflow::settings::admin

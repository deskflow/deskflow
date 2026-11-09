// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QByteArray>
#include <QString>
namespace deskflow::candidate {
inline const QString pausedKey = QStringLiteral("gui/userPaused");
inline const QString serviceAuthKey = QStringLiteral("gui/serviceAuthentication");
// Read-only validation. Does not create identities, trust, directories or permissions.
bool plainPath(const QString &path);
bool settingsPath(const QString &path);
bool childPath(const QString &root, const QString &path);
bool securityProfile(const QString &settings, const QString &certificate, bool tls, bool peers);
bool mayResume(bool autoStart, bool paused);
// Native launchd plan, never loaded or installed by this function.
QByteArray launchAgent(const QString &absoluteExecutable, bool recoverAfterCrash = true);
} // namespace deskflow::candidate

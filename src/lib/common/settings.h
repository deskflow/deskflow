/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <QDir>
#include <QObject>

/**
 * @brief The Settings class contains
 *  all the keys, enum types and other settings data
 *  to be used by DeskflowSettings
 */
class Settings
{
  Q_GADGET
public:
#if defined(Q_OS_WIN)
  inline const static auto UserSettingPath =
      QStringLiteral("%1/AppData/Local/%2/%3.ini").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("C:/ProgramData/%2/%3.ini").arg(kAppName, kAppName);
#elif defined(Q_OS_MAC)
  inline const static auto UserSettingPath =
      QStringLiteral("%1/Library/%2/%3.conf").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("/Libaray/%2/%3.conf").arg(kAppName, kAppName);
#else
  inline const static auto UserSettingPath =
      QStringLiteral("%1/.config/%2/%3.conf").arg(QDir::homePath(), kAppName, kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("/etc/%2/%3.conf").arg(kAppName, kAppName);
#endif
  struct Core
  {
    inline static const auto Scope = QStringLiteral("core/loadFromSystemScope");
  };

  inline static const QStringList validKeys = {Settings::Core::Scope};
};

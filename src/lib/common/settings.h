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
  inline const static auto UserDir = QStringLiteral("%1/AppData/Local/%2").arg(QDir::homePath(), kAppName);
  inline const static auto UserSettingPath = QStringLiteral("%1/%2.conf").arg(UserDir, kAppName);
  inline const static auto SystemDir = QStringLiteral("C:/ProgramData/%1").arg(kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("%1/%2.conf").arg(SystemDir, kAppName);
#elif defined(Q_OS_MAC)
  inline const static auto UserDir = QStringLiteral("%1/Library/%2").arg(QDir::homePath(), kAppName);
  inline const static auto UserSettingPath = QStringLiteral("%1/%2.conf").arg(UserDir, kAppName);
  inline const static auto SystemDir = QStringLiteral("/Library/%1").arg(kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("%1/%2.conf").arg(SystemDir, kAppName);
#else
  inline const static auto UserDir = QStringLiteral("%1/.config/%2").arg(QDir::homePath(), kAppName);
  inline const static auto UserSettingPath = QStringLiteral("%1/%2.conf").arg(UserDir, kAppName);
  inline const static auto SystemDir = QStringLiteral("/etc/%1").arg(kAppName);
  inline const static auto SystemSettingPath = QStringLiteral("%1/%2.conf").arg(SystemDir, kAppName);
#endif

  struct Client
  {
    inline static const auto Binary = QStringLiteral("client/binary");
    inline static const auto InvertScrollDirection = QStringLiteral("client/invertscolldirection");
    inline static const auto LanguageSync = QStringLiteral("client/languageSync");
    inline static const auto RemoteHost = QStringLiteral("client/remotehost");
  };
  struct Core
  {
    inline static const auto Interface = QStringLiteral("core/interface");
    inline static const auto LastVersion = QStringLiteral("core/lastVersion");
    inline static const auto Port = QStringLiteral("core/port");
    inline static const auto PreventSleep = QStringLiteral("core/preventSleep");
    inline static const auto Scope = QStringLiteral("core/loadFromSystemScope");
    inline static const auto StartedBefore = QStringLiteral("core/startedBefore");
  };
  struct Gui
  {
    inline static const auto Autohide = QStringLiteral("gui/autoHide");
    inline static const auto AutoUpdateCheck = QStringLiteral("gui/enableUpdateCheck");
    inline static const auto CloseReminder = QStringLiteral("gui/closeReminder");
    inline static const auto CloseToTray = QStringLiteral("gui/closeToTray");
    inline static const auto LogExpanded = QStringLiteral("gui/logExpanded");
    inline static const auto SymbolicTrayIcon = QStringLiteral("gui/symbolicTrayIcon");
    inline static const auto WindowGeometry = QStringLiteral("gui/windowGeometry");
  };
  struct Log
  {
    inline static const auto Level = QStringLiteral("log/level");
    inline static const auto ToFile = QStringLiteral("log/toFile");
  };
  struct Security
  {
    inline static const auto CheckPeers = QStringLiteral("security/checkpeerfingerprints");
    inline static const auto Certificate = QStringLiteral("security/certificate");
    inline static const auto KeySize = QStringLiteral("security/keySize");
    inline static const auto TlsEnabled = QStringLiteral("security/tlsEnabled");
  };
  struct Server
  {
    inline static const auto Binary = QStringLiteral("server/binary");
    inline static const auto ExternalConfig = QStringLiteral("server/externalConfig");
    inline static const auto ExternalConfigFile = QStringLiteral("server/externalConfigFile");
  };

  // clang-format off
  inline static const QStringList logLevels = {
     QStringLiteral("INFO")
    , QStringLiteral("DEBUG")
    , QStringLiteral("DEBUG1")
    , QStringLiteral("DEBUG2")
  };

  inline static const QStringList validKeys = {
      Settings::Client::Binary
    , Settings::Client::InvertScrollDirection
    , Settings::Client::LanguageSync
    , Settings::Client::RemoteHost
    , Settings::Core::Interface
    , Settings::Core::LastVersion
    , Settings::Core::Port
    , Settings::Core::PreventSleep
    , Settings::Core::Scope
    , Settings::Core::StartedBefore
    , Settings::Log::Level
    , Settings::Log::ToFile
    , Settings::Gui::Autohide
    , Settings::Gui::AutoUpdateCheck
    , Settings::Gui::CloseReminder
    , Settings::Gui::CloseToTray
    , Settings::Gui::LogExpanded
    , Settings::Gui::SymbolicTrayIcon
    , Settings::Gui::WindowGeometry
    , Settings::Security::Certificate
    , Settings::Security::CheckPeers
    , Settings::Security::KeySize
    , Settings::Security::TlsEnabled
    , Settings::Server::Binary
    , Settings::Server::ExternalConfig
    , Settings::Server::ExternalConfigFile
  };
  // clang-format on
};

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2016 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QSettings>

#include <QDir>

#include "common/Constants.h"
#include "common/QSettingsProxy.h"

class Settings : public QObject
{
  Q_OBJECT
public:
#if defined(Q_OS_WIN)
  inline const static auto UserDir = QStringLiteral("%1/AppData/Roaming/%2").arg(QDir::homePath(), kAppName);
  inline const static auto SystemDir = QStringLiteral("%1ProgramData/%2").arg(QDir::rootPath(), kAppName);
#elif defined(Q_OS_MAC)
  inline const static auto UserDir = QStringLiteral("%1/Library/%2").arg(QDir::homePath(), kAppName);
  inline const static auto SystemDir = QStringLiteral("/Library/%1").arg(kAppName);
#else
  inline const static auto UserDir = QStringLiteral("%1/.config/%2").arg(QDir::homePath(), kAppName);
  inline const static auto SystemDir = QStringLiteral("/etc/%1").arg(kAppName);
#endif

  inline const static auto UserSettingFile = QStringLiteral("%1/%2.conf").arg(UserDir, kAppName);
  inline const static auto SystemSettingFile = QStringLiteral("%1/%2.conf").arg(SystemDir, kAppName);

  struct Client
  {
    inline static const auto InvertScrollDirection = QStringLiteral("client/invertScrollDirection");
    inline static const auto ScrollSpeed = QStringLiteral("client/yscroll");
    inline static const auto LanguageSync = QStringLiteral("client/languageSync");
    inline static const auto RemoteHost = QStringLiteral("client/remoteHost");
    inline static const auto XdpRestoreToken = QStringLiteral("client/xdpRestoreToken");
  };
  struct Core
  {
    inline static const auto CoreMode = QStringLiteral("core/coreMode");
    inline static const auto Interface = QStringLiteral("core/interface");
    inline static const auto LastVersion = QStringLiteral("core/lastVersion");
    inline static const auto Port = QStringLiteral("core/port");
    inline static const auto PreventSleep = QStringLiteral("core/preventSleep");
    inline static const auto ProcessMode = QStringLiteral("core/processMode");
    inline static const auto ScreenName = QStringLiteral("core/screenName");
    inline static const auto StartedBefore = QStringLiteral("core/startedBefore");
    inline static const auto UpdateUrl = QStringLiteral("core/updateUrl");
    inline static const auto Display = QStringLiteral("core/display");
    inline static const auto UseHooks = QStringLiteral("core/useHooks");
    inline static const auto Language = QStringLiteral("core/language");
  };
  struct Daemon
  {
    inline static const auto Command = QStringLiteral("daemon/command");
    inline static const auto Elevate = QStringLiteral("daemon/elevate");
    inline static const auto LogFile = QStringLiteral("daemon/logFile");
    inline static const auto LogLevel = QStringLiteral("daemon/logLevel");
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
    inline static const auto ShowGenericClientFailureDialog = QStringLiteral("gui/showGenericClientFailureDialog");
  };
  struct Log
  {
    inline static const auto File = QStringLiteral("log/file");
    inline static const auto Level = QStringLiteral("log/level");
    inline static const auto ToFile = QStringLiteral("log/toFile");
    inline static const auto GuiDebug = QStringLiteral("log/guiDebug");
  };
  struct Security
  {
    inline static const auto CheckPeers = QStringLiteral("security/checkPeerFingerprints");
    inline static const auto Certificate = QStringLiteral("security/certificate");
    inline static const auto KeySize = QStringLiteral("security/keySize");
    inline static const auto TlsEnabled = QStringLiteral("security/tlsEnabled");
  };
  struct Server
  {
    inline static const auto ExternalConfig = QStringLiteral("server/externalConfig");
    inline static const auto ExternalConfigFile = QStringLiteral("server/externalConfigFile");
  };

  // Enums types used in settings
  // The use of enum classes is not use for these
  // enum classes are more specific when used with QVariant
  // This leads longer function calls in code
  // and longer more cryptic output in the settings file
  // The using of standard enum will just write ints
  // and we can read / write them as if they were ints
  enum ProcessMode
  {
    Service,
    Desktop
  };
  Q_ENUM(ProcessMode)

  enum CoreMode
  {
    None,
    Client,
    Server
  };
  Q_ENUM(CoreMode)

  static Settings *instance();
  static void setSettingsFile(const QString &settingsFile = QString());
  static void setValue(const QString &key = QString(), const QVariant &value = QVariant());
  static QVariant value(const QString &key = QString());
  static void restoreDefaultSettings();
  static QVariant defaultValue(const QString &key);
  static bool isWritable();
  static bool isPortableMode();
  static QString settingsFile();
  static QString settingsPath();
  static QString tlsDir();
  static QString tlsLocalDb();
  static QString tlsTrustedServersDb();
  static QString tlsTrustedClientsDb();
  static QString logLevelText();
  static QSettingsProxy &proxy();
  static void save(bool emitSaving = true);
  static QStringList validKeys();
  static int logLevelToInt(const QString &level = "INFO");
  static QString portableSettingsFile();

Q_SIGNALS:
  void settingsChanged(const QString key);
  void serverSettingsChanged();

private:
  explicit Settings(QObject *parent = nullptr);
  Settings *operator=(Settings &other) = delete;
  Settings(const Settings &other) = delete;
  ~Settings() override = default;
  void cleanSettings();

  QSettings *m_settings = nullptr;
  std::shared_ptr<QSettingsProxy> m_settingsProxy;

  // clang-format off
  inline static const QStringList m_logLevels = {
      QStringLiteral("FATAL")
    , QStringLiteral("ERROR")
    , QStringLiteral("WARNING")
    , QStringLiteral("NOTE")
    , QStringLiteral("INFO")
    , QStringLiteral("DEBUG")
    , QStringLiteral("DEBUG1")
    , QStringLiteral("DEBUG2")
  };

  inline static const QStringList m_validKeys = {
      Settings::Client::InvertScrollDirection
    , Settings::Client::LanguageSync
    , Settings::Client::RemoteHost
    , Settings::Client::ScrollSpeed
    , Settings::Client::XdpRestoreToken
    , Settings::Core::CoreMode
    , Settings::Core::Interface
    , Settings::Core::LastVersion
    , Settings::Core::Port
    , Settings::Core::PreventSleep
    , Settings::Core::ProcessMode
    , Settings::Core::ScreenName
    , Settings::Core::StartedBefore
    , Settings::Core::UpdateUrl
    , Settings::Core::Display
    , Settings::Core::UseHooks
    , Settings::Core::Language
    , Settings::Daemon::Command
    , Settings::Daemon::Elevate
    , Settings::Daemon::LogFile
    , Settings::Log::File
    , Settings::Log::Level
    , Settings::Log::ToFile
    , Settings::Log::GuiDebug
    , Settings::Gui::Autohide
    , Settings::Gui::AutoUpdateCheck
    , Settings::Gui::CloseReminder
    , Settings::Gui::CloseToTray
    , Settings::Gui::LogExpanded
    , Settings::Gui::SymbolicTrayIcon
    , Settings::Gui::WindowGeometry
    , Settings::Gui::ShowGenericClientFailureDialog
    , Settings::Security::Certificate
    , Settings::Security::CheckPeers
    , Settings::Security::KeySize
    , Settings::Security::TlsEnabled
    , Settings::Server::ExternalConfig
    , Settings::Server::ExternalConfigFile
  };

  // When checking the default values this list contains the ones that default to false.
  inline static const QStringList m_defaultFalseValues = {
      Settings::Gui::Autohide
    , Settings::Core::StartedBefore
    , Settings::Core::PreventSleep
    , Settings::Server::ExternalConfig
    , Settings::Client::InvertScrollDirection
    , Settings::Log::ToFile
    , Settings::Log::GuiDebug
  };

  // When checking the default values this list contains the ones that default to true.
  inline static const QStringList m_defaultTrueValues = {
      Settings::Core::UseHooks
    , Settings::Client::LanguageSync
    , Settings::Gui::CloseToTray
    , Settings::Gui::CloseReminder
    , Settings::Gui::LogExpanded
    , Settings::Gui::SymbolicTrayIcon
    , Settings::Gui::ShowGenericClientFailureDialog
    , Settings::Security::TlsEnabled
    , Settings::Security::CheckPeers
  };
  // clang-format on
};

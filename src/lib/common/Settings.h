/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2016 - 2025 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QSettings>

#include <QDir>

#include "common/Constants.h"
#include "common/NetworkProtocol.h"
#include "common/QSettingsProxy.h"

class Settings : public QObject
{
  Q_OBJECT
public:
#if defined(Q_OS_WIN)
  inline const static auto UserDir = QStringLiteral("%1/AppData/Roaming/%2").arg(QDir::homePath(), kAppName);
  inline const static auto SystemDir = QStringLiteral("%1ProgramData/%2").arg(QDir::rootPath(), kAppName);
#elif defined(Q_OS_MACOS)
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
    inline static const auto DynamicConnectionRetry = QStringLiteral("client/dynamicConnectionInterval");
    inline static const auto InvertYScroll = QStringLiteral("client/invertYScroll");
    inline static const auto InvertXScroll = QStringLiteral("client/invertXScroll");
    inline static const auto YScrollScale = QStringLiteral("client/yScrollScale");
    inline static const auto XScrollScale = QStringLiteral("client/xScrollScale");
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
    inline static const auto ComputerName = QStringLiteral("core/computerName");
    inline static const auto Display = QStringLiteral("core/display");
    inline static const auto UseHooks = QStringLiteral("core/useHooks");
    inline static const auto Language = QStringLiteral("core/language");
    inline static const auto EnableEnterCommand = QStringLiteral("core/enableEnterCommand");
    inline static const auto ScreenEnterCommand = QStringLiteral("core/enterCommand");
    inline static const auto EnableExitCommand = QStringLiteral("core/enableExitCommand");
    inline static const auto ScreenExitCommand = QStringLiteral("core/exitCommand");

    // TODO: REMOVE In 2.0
    inline static const auto ScreenName = QStringLiteral("core/screenName"); // Replaced By ComputerName
  };
  struct Daemon
  {
    inline static const auto ConfigFile = QStringLiteral("daemon/configFile");
    inline static const auto Elevate = QStringLiteral("daemon/elevate");
    inline static const auto LogFile = QStringLiteral("daemon/logFile");
    inline static const auto LogLevel = QStringLiteral("daemon/logLevel");
  };
  struct Gui
  {
    inline static const auto Autohide = QStringLiteral("gui/autoHide");
    inline static const auto AutoStartCore = QStringLiteral("gui/startCoreWithGui");
    inline static const auto AutoUpdateCheck = QStringLiteral("gui/enableUpdateCheck");
    inline static const auto UpdateCheckUrl = QStringLiteral("gui/updateCheckUrl");
    inline static const auto CloseReminder = QStringLiteral("gui/closeReminder");
    inline static const auto CloseToTray = QStringLiteral("gui/closeToTray");
    inline static const auto LogExpanded = QStringLiteral("gui/logExpanded");
    inline static const auto SymbolicTrayIcon = QStringLiteral("gui/symbolicTrayIcon");
    inline static const auto WindowGeometry = QStringLiteral("gui/windowGeometry");
    inline static const auto ShownFirstConnectedMessage = QStringLiteral("gui/shownFirstConnectedMessage");
    inline static const auto ShownServerFirstStartMessage = QStringLiteral("gui/shownServerFirstStartMessage");
    inline static const auto ShowVersionInTitle = QStringLiteral("gui/showVersionInTitle");
    inline static const auto IgnoreMissingKeyboardLayouts = QStringLiteral("gui/ignoreMissingKeyboardLayouts");
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
    inline static const auto ClipboardSize = QStringLiteral("server/clipboardSize");
    inline static const auto DefaultLockToComputerState = QStringLiteral("server/defaultLockToComputerState");
    inline static const auto DisableLockToComputer = QStringLiteral("server/disableLockToComputer");
    inline static const auto EnableClipboard = QStringLiteral("server/enableClipboard");
    inline static const auto EnableHeatbeat = QStringLiteral("server/enableHeatbeat");
    inline static const auto EnableSwitchDelay = QStringLiteral("server/enableSwitchDelay");
    inline static const auto EnableSwitchDoubleTap = QStringLiteral("server/enableSwitchDoubleTap");
    inline static const auto ExternalConfig = QStringLiteral("server/externalConfig");
    inline static const auto ExternalConfigFile = QStringLiteral("server/externalConfigFile");
    inline static const auto GridHeight = QStringLiteral("server/gridHeight");
    inline static const auto GridWidth = QStringLiteral("server/gridWidth");
    inline static const auto Heartbeat = QStringLiteral("server/heartbeat");
    inline static const auto Protocol = QStringLiteral("server/protocol");
    inline static const auto RelativeMouseMoves = QStringLiteral("server/relativeMouseMoves");
    inline static const auto SwitchDelay = QStringLiteral("server/switchDelay");
    inline static const auto SwitchDoubleTap = QStringLiteral("server/switchDoubleTap");
    inline static const auto Win32KeepForeground = QStringLiteral("server/win32KeepForeground");
    inline static const auto XdpRestoreToken = QStringLiteral("server/xdpRestoreToken");
  };

  struct Screen
  {
    inline static const auto Aliases = QStringLiteral("screen_%1/aliases");
  };

  // Track Removed keys to make upgrading config easier
  // REMOVE FOR 2.0
  struct InternalConfig
  {
    inline static const auto NumRows = QStringLiteral("internalConfig/numRows");
    inline static const auto NumColumns = QStringLiteral("internalConfig/numColumns");
    inline static const auto ClipboardSharing = QStringLiteral("internalConfig/clipboardSharing");
    inline static const auto Heatbeat = QStringLiteral("internalConfig/heartbeat");
    inline static const auto SwitchDelay = QStringLiteral("internalConfig/switchDelay");
    inline static const auto HasHeartbeat = QStringLiteral("internalConfig/hasHeartbeat");
    inline static const auto HasSwitchDelay = QStringLiteral("internalConfig/hasSwitchDelay");
    inline static const auto HasSwitchDoubleTap = QStringLiteral("internalConfig/hasSwitchDoubleTap");
    inline static const auto DefaultLockToScreenState = QStringLiteral("internalConfig/defaultLockToScreenState");
    inline static const auto DisableLockToScreen = QStringLiteral("internalConfig/disableLockToScreen");
    inline static const auto SwitchDoubleTapDelay = QStringLiteral("internalConfig/switchDoubleTap");
    inline static const auto Win32KeepForeground = QStringLiteral("internalConfig/win32KeepForeground");
    inline static const auto RelativeMouseMoves = QStringLiteral("internalConfig/relativeMouseMoves");
    inline static const auto Protocol = QStringLiteral("internalConfig/protocol");
    inline static const auto ClipboardSharingSize = QStringLiteral("internalConfig/clipboardSharingSize");
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
  static void setStateFile(const QString &stateFile = QString());
  static void setValue(const QString &key = QString(), const QVariant &value = QVariant());
  static QVariant value(const QString &key = QString());
  static void restoreDefaultSettings();
  static QVariant defaultValue(const QString &key);
  static bool isServerConfigFileReadable();
  static bool isWritable();
  static bool isPortableMode();
  static QString settingsFile();
  static QString settingsPath();
  static QString serverConfigFile();
  static QString tlsDir();
  static QString tlsTrustedServersDb();
  static QString tlsTrustedClientsDb();
  static QString logLevelText();
  static QSettingsProxy &proxy();
  static NetworkProtocol networkProtocol();
  static void save(bool emitSaving = true);
  static QStringList validKeys();
  static QStringList validGroups();
  static QString portableSettingsFile();
  static void removeUnknownScreens(const QStringList &knownScreens);

Q_SIGNALS:
  void settingsChanged(const QString key);
  void serverSettingsChanged();

private:
  explicit Settings(QObject *parent = nullptr);
  Settings *operator=(Settings &other) = delete;
  Settings(const Settings &other) = delete;
  ~Settings() override = default;

  /**
   * @brief This method uses the Settings::m_upgradeMap, keys are upgraded if the oldkey is found and the newKey is not
   * This method is run when settings is created before cleaning the settings and when you change settings files
   * It does not remove any keys, only copies the old value to the new setings key
   */
  void upgradeSettings();
  void cleanSettings();
  void cleanStateSettings();

  /**
   * @brief write an initial computer name
   */
  void setupComputerName();

  /**
   * @brief cleanComputerName ensure a valid computerName from the provided one
   * @param name any string to be used as the computerName
   * @return a valid computerName
   */
  static QString cleanComputerName(const QString &name);

  QSettings *m_settings = nullptr;
  QSettings *m_stateSettings = nullptr;
  std::shared_ptr<QSettingsProxy> m_settingsProxy;

  // clang-format off
  inline static const QStringList m_validGroup = {
      QStringLiteral("client")
    , QStringLiteral("core")
    , QStringLiteral("daemon")
    , QStringLiteral("gui")
    , QStringLiteral("log")
    , QStringLiteral("security")
    , QStringLiteral("server")
    , QStringLiteral("internalConfig")
  };

  inline static const QStringList m_validKeys = {
      Settings::Client::DynamicConnectionRetry
    , Settings::Client::InvertYScroll
    , Settings::Client::InvertXScroll
    , Settings::Client::LanguageSync
    , Settings::Client::RemoteHost
    , Settings::Client::YScrollScale
    , Settings::Client::XScrollScale
    , Settings::Client::XdpRestoreToken
    , Settings::Core::CoreMode
    , Settings::Core::Interface
    , Settings::Core::LastVersion
    , Settings::Core::Port
    , Settings::Core::PreventSleep
    , Settings::Core::ProcessMode
    , Settings::Core::EnableEnterCommand
    , Settings::Core::EnableExitCommand
    , Settings::Core::ScreenEnterCommand
    , Settings::Core::ScreenExitCommand
    , Settings::Core::ScreenName
    , Settings::Core::ComputerName
    , Settings::Core::Display
    , Settings::Core::UseHooks
    , Settings::Core::Language
    , Settings::Daemon::ConfigFile
    , Settings::Daemon::Elevate
    , Settings::Daemon::LogFile
    , Settings::Daemon::LogLevel
    , Settings::Log::File
    , Settings::Log::Level
    , Settings::Log::ToFile
    , Settings::Log::GuiDebug
    , Settings::Gui::Autohide
    , Settings::Gui::AutoStartCore
    , Settings::Gui::AutoUpdateCheck
    , Settings::Gui::UpdateCheckUrl
    , Settings::Gui::CloseReminder
    , Settings::Gui::CloseToTray
    , Settings::Gui::LogExpanded
    , Settings::Gui::SymbolicTrayIcon
    , Settings::Gui::WindowGeometry
    , Settings::Gui::ShownFirstConnectedMessage
    , Settings::Gui::ShownServerFirstStartMessage
    , Settings::Gui::ShowVersionInTitle
    , Settings::Gui::IgnoreMissingKeyboardLayouts
    , Settings::Security::Certificate
    , Settings::Security::CheckPeers
    , Settings::Security::KeySize
    , Settings::Security::TlsEnabled
    , Settings::Server::ClipboardSize
    , Settings::Server::DefaultLockToComputerState
    , Settings::Server::DisableLockToComputer
    , Settings::Server::EnableClipboard
    , Settings::Server::EnableHeatbeat
    , Settings::Server::EnableSwitchDelay
    , Settings::Server::EnableSwitchDoubleTap
    , Settings::Server::ExternalConfig
    , Settings::Server::ExternalConfigFile
    , Settings::Server::GridHeight
    , Settings::Server::GridWidth
    , Settings::Server::Heartbeat
    , Settings::Server::Protocol
    , Settings::Server::RelativeMouseMoves
    , Settings::Server::SwitchDelay
    , Settings::Server::SwitchDoubleTap
    , Settings::Server::Win32KeepForeground
    , Settings::Server::XdpRestoreToken
  };

  // When checking the default values this list contains the ones that default to false.
  inline static const QStringList m_defaultFalseValues = {
      Settings::Gui::Autohide
    , Settings::Gui::AutoStartCore
    , Settings::Gui::ShownFirstConnectedMessage
    , Settings::Gui::ShownServerFirstStartMessage
    , Settings::Gui::ShowVersionInTitle
    , Settings::Gui::IgnoreMissingKeyboardLayouts
    , Settings::Core::PreventSleep
    , Settings::Core::EnableEnterCommand
    , Settings::Core::EnableExitCommand
    , Settings::Client::DynamicConnectionRetry
    , Settings::Client::InvertYScroll
    , Settings::Client::InvertXScroll
    , Settings::Log::ToFile
    , Settings::Log::GuiDebug
    , Settings::Server::DefaultLockToComputerState
    , Settings::Server::DisableLockToComputer
    , Settings::Server::EnableHeatbeat
    , Settings::Server::EnableSwitchDelay
    , Settings::Server::EnableSwitchDoubleTap
    , Settings::Server::ExternalConfig
    , Settings::Server::RelativeMouseMoves
  };

  // When checking the default values this list contains the ones that default to true.
  inline static const QStringList m_defaultTrueValues = {
      Settings::Core::UseHooks
    , Settings::Client::LanguageSync
    , Settings::Gui::CloseToTray
    , Settings::Gui::CloseReminder
    , Settings::Gui::LogExpanded
    , Settings::Gui::SymbolicTrayIcon
    , Settings::Security::TlsEnabled
    , Settings::Security::CheckPeers
    , Settings::Server::EnableClipboard
    , Settings::Server::Win32KeepForeground
  };

  // Settings saved in our State file
  inline static const QStringList m_stateKeys = { Settings::Gui::WindowGeometry };

  // Contains settings keys to be upgraded.
  inline static const QMap<QString, QString> m_upgradedMap = {
    /*             OLD KEY                        NEW KEY          */
      {Core::ScreenName, Core::ComputerName}
    , {InternalConfig::NumColumns, Server::GridWidth}
    , {InternalConfig::NumRows, Server::GridHeight}
    , {InternalConfig::Heatbeat, Server::Heartbeat}
    , {InternalConfig::SwitchDelay, Server::SwitchDelay}
    , {InternalConfig::HasHeartbeat, Server::EnableHeatbeat}
    , {InternalConfig::HasSwitchDelay, Server::EnableSwitchDelay}
    , {InternalConfig::HasSwitchDoubleTap, Server::EnableSwitchDoubleTap}
    , {InternalConfig::ClipboardSharing, Server::EnableClipboard}
    , {InternalConfig::DisableLockToScreen, Server::DisableLockToComputer}
    , {InternalConfig::DefaultLockToScreenState, Server::DefaultLockToComputerState}
    , {InternalConfig::SwitchDoubleTapDelay, Server::SwitchDoubleTap}
    , {InternalConfig::Win32KeepForeground, Server::Win32KeepForeground}
    , {InternalConfig::RelativeMouseMoves, Server::RelativeMouseMoves}
    , {InternalConfig::Protocol, Server::Protocol}
    , {InternalConfig::ClipboardSharingSize, Server::ClipboardSize}
  };

// Contains settings removed from server-configuration file
  inline static const QStringList m_oldServerConfigKeys = {
      InternalConfig::DefaultLockToScreenState
    , InternalConfig::DisableLockToScreen
    , InternalConfig::ClipboardSharing
    , InternalConfig::ClipboardSharingSize
    , InternalConfig::HasHeartbeat
    , InternalConfig::HasSwitchDelay
    , InternalConfig::HasSwitchDoubleTap
    , InternalConfig::Heatbeat
    , InternalConfig::NumColumns
    , InternalConfig::NumRows
    , InternalConfig::RelativeMouseMoves
    , InternalConfig::SwitchDelay
    , InternalConfig::SwitchDoubleTapDelay
    , InternalConfig::Win32KeepForeground
    , InternalConfig::Protocol
    , QStringLiteral("internalConfig/switchCorner")
  };
  // clang-format on
};

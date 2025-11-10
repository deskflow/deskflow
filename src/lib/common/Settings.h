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

using namespace Qt::StringLiterals;

class Settings : public QObject
{
  Q_OBJECT
public:
#if defined(Q_OS_WIN)
  inline const static auto UserDir = u"%1/AppData/Roaming/%2"_s.arg(QDir::homePath(), kAppName);
  inline const static auto SystemDir = u"%1ProgramData/%2"_s.arg(QDir::rootPath(), kAppName);
#elif defined(Q_OS_MAC)
  inline const static auto UserDir = u"%1/Library/%2"_s.arg(QDir::homePath(), kAppName);
  inline const static auto SystemDir = u"/Library/%1"_s.arg(kAppName);
#else
  inline const static auto UserDir = u"%1/.config/%2"_s.arg(QDir::homePath(), kAppName);
  inline const static auto SystemDir = u"/etc/%1"_s.arg(kAppName);
#endif

  inline const static auto UserSettingFile = u"%1/%2.conf"_s.arg(UserDir, kAppName);
  inline const static auto SystemSettingFile = u"%1/%2.conf"_s.arg(SystemDir, kAppName);

  struct Client
  {
    inline static const auto InvertScrollDirection = u"client/invertScrollDirection"_s;
    inline static const auto ScrollSpeed = u"client/yscroll"_s;
    inline static const auto LanguageSync = u"client/languageSync"_s;
    inline static const auto RemoteHost = u"client/remoteHost"_s;
    inline static const auto XdpRestoreToken = u"client/xdpRestoreToken"_s;
  };
  struct Core
  {
    inline static const auto CoreMode = u"core/coreMode"_s;
    inline static const auto Interface = u"core/interface"_s;
    inline static const auto LastVersion = u"core/lastVersion"_s;
    inline static const auto Port = u"core/port"_s;
    inline static const auto PreventSleep = u"core/preventSleep"_s;
    inline static const auto ProcessMode = u"core/processMode"_s;
    inline static const auto ScreenName = u"core/screenName"_s;
    inline static const auto StartedBefore = u"core/startedBefore"_s;
    inline static const auto UpdateUrl = u"core/updateUrl"_s;
    inline static const auto Display = u"core/display"_s;
    inline static const auto UseHooks = u"core/useHooks"_s;
    inline static const auto Language = u"core/language"_s;
  };
  struct Daemon
  {
    inline static const auto Command = u"daemon/command"_s;
    inline static const auto Elevate = u"daemon/elevate"_s;
    inline static const auto LogFile = u"daemon/logFile"_s;
    inline static const auto LogLevel = u"daemon/logLevel"_s;
  };
  struct Gui
  {
    inline static const auto Autohide = u"gui/autoHide"_s;
    inline static const auto AutoUpdateCheck = u"gui/enableUpdateCheck"_s;
    inline static const auto CloseReminder = u"gui/closeReminder"_s;
    inline static const auto CloseToTray = u"gui/closeToTray"_s;
    inline static const auto LogExpanded = u"gui/logExpanded"_s;
    inline static const auto SymbolicTrayIcon = u"gui/symbolicTrayIcon"_s;
    inline static const auto WindowGeometry = u"gui/windowGeometry"_s;
    inline static const auto ShowGenericClientFailureDialog = u"gui/showGenericClientFailureDialog"_s;
  };
  struct Log
  {
    inline static const auto File = u"log/file"_s;
    inline static const auto Level = u"log/level"_s;
    inline static const auto ToFile = u"log/toFile"_s;
    inline static const auto GuiDebug = u"log/guiDebug"_s;
  };
  struct Security
  {
    inline static const auto CheckPeers = u"security/checkPeerFingerprints"_s;
    inline static const auto Certificate = u"security/certificate"_s;
    inline static const auto KeySize = u"security/keySize"_s;
    inline static const auto TlsEnabled = u"security/tlsEnabled"_s;
  };
  struct Server
  {
    inline static const auto ExternalConfig = u"server/externalConfig"_s;
    inline static const auto ExternalConfigFile = u"server/externalConfigFile"_s;
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
  static int logLevelToInt(const QString &level = u"INFO"_s);
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
  void cleanStateSettings();

  QSettings *m_settings = nullptr;
  QSettings *m_stateSettings = nullptr;
  std::shared_ptr<QSettingsProxy> m_settingsProxy;

  // clang-format off
  inline static const QStringList m_logLevels = {
      u"FATAL"_s
    , u"ERROR"_s
    , u"WARNING"_s
    , u"NOTE"_s
    , u"INFO"_s
    , u"DEBUG"_s
    , u"DEBUG1"_s
    , u"DEBUG2"_s
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

  // Settings saved in our State file
  inline static const QStringList m_stateKeys = { Settings::Gui::WindowGeometry };
  // clang-format on
};

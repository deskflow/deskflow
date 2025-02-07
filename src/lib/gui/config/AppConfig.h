/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ElevateMode.h"
#include "IAppConfig.h"
#include "IConfigScopes.h"
#include "common/constants.h"
#include "gui/paths.h"

#include <QDir>
#include <QHostInfo>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVariant>
#include <optional>

namespace deskflow::gui {

const ElevateMode kDefaultElevateMode = ElevateMode::kAutomatic;
const QString kDefaultLogFile = QStringLiteral("%1.log").arg(kAppId);
const int kDefaultTlsKeyLength = 2048;

#if defined(Q_OS_WIN)
const ProcessMode kDefaultProcessMode = ProcessMode::kService;
#else
const ProcessMode kDefaultProcessMode = ProcessMode::kDesktop;
#endif // Q_OS_WIN

} // namespace deskflow::gui

/**
 * @brief Simply reads and writes app settings.
 *
 * Important: Maintain a clear separation of concerns and keep it simple.
 * It is tempting to add logic (e.g. license checks) to this class since it
 * instance is widely accessible, but that has previously led to this class
 * becoming a god object.
 */
class AppConfig : public QObject, public deskflow::gui::IAppConfig
{
  using ProcessMode = deskflow::gui::ProcessMode;
  using IConfigScopes = deskflow::gui::IConfigScopes;

  Q_OBJECT

private:
  enum class Setting
  {
    kScreenName = 0,
    kPort = 1,
    kInterface = 2,
    kLogLevel = 3,
    kLogToFile = 4,
    kLogFilename = 5,
    kWizardLastRun = 6,
    kStartedBefore = 7,
    kElevateModeLegacy = 8,
    kElevateMode = 9,
    // 10 = edition, obsolete (related to obsolete licensing)
    kTlsEnabled = 11,
    kAutoHide = 12,
    // 13 = serial key, obsolete
    kLastVersion = 14,
    // 15 = last expire time, obsolete
    // 16 = activation has run, obsolete
    // 17 = minimize to tray, obsolete
    // 18 = activate email, obsolete
    kLoadSystemSettings = 19,
    kServerGroupChecked = 20,
    kUseExternalConfig = 21,
    kConfigFile = 22,
    kUseInternalConfig = 23,
    kClientGroupChecked = 24,
    kServerHostname = 25,
    kTlsCertPath = 26,
    kTlsKeyLength = 27,
    kPreventSleep = 28,
    kLanguageSync = 29,
    kInvertScrollDirection = 30,
    // 31 = guid, obsolete
    // 32 = license registry url, obsolete
    // 33 = license next check, obsolete
    // 34 = InvertConnection, obsolete
    // 35 = client-host-mode, obsolete
    // 36 = server-client-mode, obsolete
    kEnableService = 37,
    kCloseToTray = 38,
    kMainWindowSize = 39,
    kMainWindowPosition = 40,
    // 41 = show dev thanks, obsolete
    kShowCloseReminder = 42,
    kEnableUpdateCheck = 43,
    kLogExpanded = 44,
    kColorfulIcon = 45,
    kRequireClientCert = 46
  };

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual QString defaultTlsCertPath() const
    {
      return deskflow::gui::paths::defaultTlsCertPath();
    }
    virtual QString hostname() const
    {
      return QHostInfo::localHostName();
    }
  };

  explicit AppConfig(IConfigScopes &scopes, std::shared_ptr<Deps> deps = std::make_shared<Deps>());

  void determineScope();

  /**
   * @brief Commits the current settings to the active scope.
   * This should only be called when the settings are about to be saved.
   */
  void commit();

  //
  // Getters (overrides)
  //

  IConfigScopes &scopes() const override;
  ProcessMode processMode() const override;
  ElevateMode elevateMode() const override;
  bool tlsEnabled() const override;
  QString tlsCertPath() const override;
  int tlsKeyLength() const override;
  QString logLevelText() const override;
  const QString &screenName() const override;
  bool logToFile() const override;
  bool preventSleep() const override;
  const QString &logFilename() const override;
  QString coreServerName() const override;
  QString coreClientName() const override;
  void persistLogDir() const override;
  bool languageSync() const override;
  bool invertScrollDirection() const override;
  int port() const override;
  bool useExternalConfig() const override;
  const QString &configFile() const override;
  const QString &networkInterface() const override;
  const QString &serverHostname() const override;
  bool isActiveScopeWritable() const override;
  bool isActiveScopeSystem() const override;
  int logLevel() const override;
  bool autoHide() const override;
  bool enableService() const override;
  bool closeToTray() const override;
  bool clientGroupChecked() const override;
  bool requireClientCerts() const override;

  //
  // Getters (new methods)
  //

  bool wizardShouldRun() const;
  bool startedBefore() const;
  QString logDir() const;
  bool serverGroupChecked() const;
  bool useInternalConfig() const;
  QString lastVersion() const;
  std::optional<QSize> mainWindowSize() const;
  std::optional<QPoint> mainWindowPosition() const;
  bool showCloseReminder() const;
  std::optional<bool> enableUpdateCheck() const;
  bool logExpanded() const;
  bool colorfulTrayIcon() const;

  //
  // Setters (overrides)
  //

  void setScreenName(const QString &s) override;
  void setPort(int i) override;
  void setNetworkInterface(const QString &s) override;
  void setLogLevel(int i) override;
  void setLogToFile(bool b) override;
  void setLogFilename(const QString &s) override;
  void setElevateMode(ElevateMode em) override;
  void setTlsEnabled(bool e) override;
  void setAutoHide(bool b) override;
  void setInvertScrollDirection(bool b) override;
  void setLanguageSync(bool b) override;
  void setPreventSleep(bool b) override;
  void setEnableService(bool enabled) override;
  void setCloseToTray(bool minimize) override;
  void setTlsCertPath(const QString &path) override;
  void setTlsKeyLength(int length) override;
  void setRequireClientCerts(bool requireClientCerts) override;

  //
  // Setters (new methods)
  //

  void setStartedBefore(bool b);
  void setWizardHasRun();
  void setServerGroupChecked(bool);
  void setUseExternalConfig(bool);
  void setConfigFile(const QString &);
  void setUseInternalConfig(bool);
  void setClientGroupChecked(bool);
  void setServerHostname(const QString &);
  void setLastVersion(const QString &version);
  void setMainWindowSize(const QSize &size);
  void setMainWindowPosition(const QPoint &position);
  void setShowCloseReminder(bool show);
  void setEnableUpdateCheck(bool value);
  void setLogExpanded(bool expanded);
  void setColorfulTrayIcon(bool color);

  /// @brief Sets the user preference to load from SystemScope.
  /// @param [in] value
  ///             True - This will set the variable and load the global scope
  ///             settings. False - This will set the variable and load the user
  ///             scope settings.
  void setLoadFromSystemScope(bool value) override;

private:
  static QString settingName(AppConfig::Setting name);

  void recall();
  void recallScreenName();
  void recallElevateMode();
  void recallFromAllScopes();
  void recallFromCurrentScope();

  /**
   * @brief Loads a setting if it exists, otherwise returns `std::nullopt`
   *
   * @param toType A function to convert the QVariant to the desired type.
   */
  template <typename T>
  std::optional<T> getFromCurrentScope(Setting name, std::function<T(const QVariant &)> toType) const;

  /**
   * @brief Sets a setting if the value is not `std::nullopt`.
   */
  template <typename T> void setInCurrentScope(Setting name, const std::optional<T> &value);

  /// @brief Sets the value of a setting
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved
  template <typename T> void setInCurrentScope(AppConfig::Setting name, T value);

  /// @brief Sets the value of a common setting
  /// which should have the same value for all scopes
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved
  template <typename T> void saveToAllScopes(AppConfig::Setting name, T value);

  QVariant getFromCurrentScope(AppConfig::Setting name, const QVariant &defaultValue = QVariant()) const;

  /**
   * @brief Finds a value by searching each scope starting with the current
   * scope.
   */
  QVariant findInAllScopes(AppConfig::Setting name, const QVariant &defaultValue = QVariant()) const;

  /// @brief This method loads config from specified scope
  /// @param [in] scope which should be loaded.
  void loadScope(IConfigScopes::Scope scope);

  /**
   * @brief Gets a TLS certificate path based on the user's profile dir.
   */
  QString defaultTlsCertPath() const;

  // Used to make the server and client names on windows.
#ifdef Q_OS_WIN
  inline static const auto s_winExeTemplate = QStringLiteral("%1.exe");
#endif
  inline static const auto s_CoreServerName = QStringLiteral("deskflow-server");
  inline static const auto s_CoreClientName = QStringLiteral("deskflow-client");
  static const char m_LogDir[];

  /// @brief Contains the string values of the settings names that will be saved
  static const char *const m_SettingsName[];

  /// @brief Core config file extention (not the Qt settings filename)
#ifdef Q_OS_WIN
  inline static const auto s_ConfigFileExt = QStringLiteral("sgc");
#else
  inline static const auto s_ConfigFileExt = QStringLiteral("conf");
#endif

  int m_Port = 24800;
  QString m_Interface = "";
  int m_LogLevel = 0;
  bool m_LogToFile = false;
  QString m_LogFilename = logDir() + deskflow::gui::kDefaultLogFile;
  int m_WizardLastRun = 0;
  bool m_StartedBefore = false;
  ElevateMode m_ElevateMode = deskflow::gui::kDefaultElevateMode;
  bool m_TlsEnabled = true;
  bool m_AutoHide = false;
  QString m_LastVersion = "";
  bool m_InvertScrollDirection = false;
  bool m_LanguageSync = true;
  bool m_PreventSleep = false;
  bool m_ServerGroupChecked = false;
  bool m_UseExternalConfig = false;
  QString m_ConfigFile = QStringLiteral("%1/%2.%3").arg(QDir::homePath(), kAppId, s_ConfigFileExt);
  bool m_UseInternalConfig = false;
  bool m_ClientGroupChecked = false;
  QString m_ServerHostname = "";
  bool m_EnableService = deskflow::gui::kDefaultProcessMode == ProcessMode::kService;
  bool m_CloseToTray = true;
  int m_TlsKeyLength = deskflow::gui::kDefaultTlsKeyLength;
  std::optional<QSize> m_MainWindowSize;
  std::optional<QPoint> m_MainWindowPosition;
  bool m_LoadFromSystemScope = false;
  bool m_ShowCloseReminder = true;
  std::optional<bool> m_EnableUpdateCheck;
  bool m_logExpanded = true;
  bool m_colorfulTrayIcon = false;
  bool m_RequireClientCert = true;

  /**
   * @brief Flag is set when any TLS is setting is changed, and is reset
   * when the TLS changed event is emitted.
   */
  bool m_TlsChanged = false;

  deskflow::gui::IConfigScopes &m_Scopes;
  std::shared_ptr<Deps> m_pDeps;
  QString m_ScreenName;
  QString m_TlsCertPath;

signals:
  void tlsChanged();
  void screenNameChanged();
  void logLevelChanged();
};

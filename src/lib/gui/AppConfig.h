/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "ConfigScopes.h"
#include "CoreInterface.h"
#include "ElevateMode.h"
#include "IAppConfig.h"

#include <QDir>
#include <QHostInfo>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVariant>
#include <optional>

enum class ProcessMode { kService, kDesktop };

const ElevateMode kDefaultElevateMode = ElevateAsNeeded;
const QString kDefaultLogFile = "synergy.log";

#if defined(Q_OS_WIN)
const ProcessMode kDefaultProcessMode = ProcessMode::kService;
#else
const ProcessMode kDefaultProcessMode = ProcessMode::kDesktop;
#endif // Q_OS_WIN

#ifdef SYNERGY_SHOW_DEV_THANKS
const bool kDefaultShowDevThanks = true;
#else
const bool kDefaultShowDevThanks = false;
#endif // SYNERGY_SHOW_DEV_THANKS

/**
 * @brief Simply reads and writes app settings.
 *
 * Important: Maintain a clear separation of concerns and keep it simple.
 * It is tempting to add logic (e.g. license checks) to this class since it
 * instance is widely accessible, but that has previously led to this class
 * becoming a god object.
 */
class AppConfig : public QObject, public synergy::gui::IAppConfig {
  Q_OBJECT

private:
  enum class Setting {
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
    // 10 = edition, obsolete (using serial key instead)
    kTlsEnabled = 11,
    kAutoHide = 12,
    kSerialKey = 13,
    kLastVersion = 14,
    // 15 = last expire time, obsolete
    kActivationHasRun = 16,
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
    kLicenseNextCheck = 33,
    kInvertConnection = 34,
    // 35 = client-host-mode, obsolete
    // 36 = server-client-mode, obsolete
    kEnableService = 37,
    kCloseToTray = 38,
    kMainWindowSize = 39,
    kMainWindowPosition = 40,
    kShowDevThanks = 41,
    kShowCloseReminder = 42,
  };

public:
  struct Deps {
    virtual ~Deps() = default;
    virtual QString profileDir() const {
      return m_coreInterface.getProfileDir();
    }
    virtual QString hostname() const { return QHostInfo::localHostName(); }

  private:
    [[no_unique_address]] CoreInterface m_coreInterface;
  };

  explicit AppConfig(
      synergy::gui::IConfigScopes &scopes,
      std::shared_ptr<Deps> deps = std::make_shared<Deps>());

  synergy::gui::IConfigScopes &scopes();
  void commit();
  void determineScope();

  /**
   * Getters
   */

  void setActivationHasRun(bool value);
  bool isCurrentScopeWritable() const;
  bool isCurrentScopeSystem() const;
  const QString &screenName() const;
  int port() const;
  const QString &networkInterface() const;
  int logLevel() const;
  bool logToFile() const;
  const QString &logFilename() const;
  QString logLevelText() const;
  ProcessMode processMode() const;
  bool wizardShouldRun() const;
  bool startedBefore() const;
  QString coreServerName() const;
  QString coreClientName() const;
  QString logDir() const;
  void persistLogDir() const;
  ElevateMode elevateMode() const;
  bool autoHide() const;
  bool invertScrollDirection() const;
  unsigned long long licenseNextCheck() const;
  bool languageSync() const;
  bool preventSleep() const;
  bool invertConnection() const;
  bool serverGroupChecked() const;
  bool useExternalConfig() const;
  const QString &configFile() const;
  bool useInternalConfig() const;
  bool clientGroupChecked() const;
  QString serverHostname() const;
  QString lastVersion() const;
  bool enableService() const;
  bool closeToTray() const;
  QString serialKey() const;
  bool activationHasRun() const;
  bool tlsEnabled() const override;
  QString tlsCertPath() const override;
  QString tlsKeyLength() const override;
  std::optional<QSize> mainWindowSize() const;
  std::optional<QPoint> mainWindowPosition() const;
  bool showDevThanks() const;
  bool showCloseReminder() const;

  /**
   * Setters
   */

  void setScreenName(const QString &s);
  void setPort(int i);
  void setNetworkInterface(const QString &s);
  void setLogLevel(int i);
  void setLogToFile(bool b);
  void setLogFilename(const QString &s);
  void setWizardHasRun();
  void setStartedBefore(bool b);
  void setElevateMode(ElevateMode em);
  void setTlsEnabled(bool e);
  void setSerialKey(const QString &serialKey);
  void clearSerialKey();
  void setAutoHide(bool b);
  void setInvertScrollDirection(bool b);
  void setLicenseNextCheck(unsigned long long);
  void setLanguageSync(bool b);
  void setPreventSleep(bool b);
  void setServerGroupChecked(bool);
  void setUseExternalConfig(bool);
  void setConfigFile(const QString &);
  void setUseInternalConfig(bool);
  void setClientGroupChecked(bool);
  void setServerHostname(const QString &);
  void setLastVersion(const QString &version);
  void setEnableService(bool enabled);
  void setCloseToTray(bool minimize);
  void setTlsCertPath(const QString &path);
  void setTlsKeyLength(const QString &length);
  void setInvertConnection(bool value);
  void setMainWindowSize(const QSize &size);
  void setMainWindowPosition(const QPoint &position);
  void setShowDevThanks(bool show);
  void setShowCloseReminder(bool show);

  /// @brief Sets the user preference to load from SystemScope.
  /// @param [in] value
  ///             True - This will set the variable and load the global scope
  ///             settings. False - This will set the variable and load the user
  ///             scope settings.
  void setLoadFromSystemScope(bool value);

private:
  static QString settingName(AppConfig::Setting name);

  void recall();
  void recallScreenName();
  void recallSerialKey();
  void recallElevateMode();
  void recallFromAllScopes();
  void recallFromCurrentScope();

  /**
   * @brief Loads a setting if it exists, otherwise returns `std::nullopt`
   *
   * @param toType A function to convert the QVariant to the desired type.
   */
  template <typename T>
  std::optional<T>
  getFromScopeOptional(Setting name, std::function<T(QVariant)> toType) const;

  /**
   * @brief Sets a setting if the value is not `std::nullopt`.
   */
  template <typename T>
  void setInCurrentScopeOptional(Setting name, const std::optional<T> &value);

  /// @brief Sets the value of a setting
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved
  template <typename T>
  void setInCurrentScope(AppConfig::Setting name, T value);

  /// @brief Sets the value of a common setting
  /// which should have the same value for all scopes
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved
  template <typename T> void saveToAllScopes(AppConfig::Setting name, T value);

  QVariant getFromScope(
      AppConfig::Setting name, const QVariant &defaultValue = QVariant());

  /**
   * @brief Finds a value by searching each scope starting with the current
   * scope.
   */
  QVariant findInAllScopes(
      AppConfig::Setting name, const QVariant &defaultValue = QVariant()) const;

  /// @brief This method loads config from specified scope
  /// @param [in] scope which should be loaded.
  void loadScope(synergy::gui::ConfigScopes::Scope scope);

  /// @brief This function sets default values
  /// for settings that shouldn't be copied from between scopes.
  void setDefaultValues();

  /**
   * @brief Gets a TLS certificate path based on the user's profile dir.
   */
  QString defaultTlsCertPath() const;

  std::shared_ptr<Deps> m_pDeps;
  synergy::gui::IConfigScopes &m_scopes;
  QString m_ScreenName;
  int m_Port = 24800;
  QString m_Interface = "";
  int m_LogLevel = 0;
  bool m_LogToFile = false;
  QString m_LogFilename = logDir() + kDefaultLogFile;
  int m_WizardLastRun = 0;
  bool m_StartedBefore = false;
  ElevateMode m_ElevateMode = kDefaultElevateMode;
  QString m_ActivateEmail = "";
  bool m_TlsEnabled = true;
  bool m_AutoHide = false;
  QString m_SerialKey = "";
  QString m_LastVersion = "";
  unsigned long long m_licenseNextCheck = 0;
  bool m_ActivationHasRun = false;
  bool m_InvertScrollDirection = false;
  bool m_LanguageSync = true;
  bool m_PreventSleep = false;
  bool m_InvertConnection = false;
  bool m_ServerGroupChecked = false;
  bool m_UseExternalConfig = false;
  QString m_ConfigFile = QDir::homePath() + "/" + m_ConfigFilename;
  bool m_UseInternalConfig = false;
  bool m_ClientGroupChecked = false;
  QString m_ServerHostname = "";
  bool m_EnableService = kDefaultProcessMode == ProcessMode::kService;
  bool m_CloseToTray = true;
  QString m_TlsCertPath = defaultTlsCertPath();
  QString m_TlsKeyLength = "2048";
  std::optional<QSize> m_MainWindowSize;
  std::optional<QPoint> m_MainWindowPosition;
  bool m_ShowDevThanks = kDefaultShowDevThanks;
  bool m_LoadFromSystemScope = false;
  bool m_ShowCloseReminder = true;

  /**
   * @brief Flag is set when any TLS is setting is changed, and is reset
   * when the TLS changed event is emitted.
   */
  bool m_TlsChanged = false;

  static const char m_CoreServerName[];
  static const char m_CoreClientName[];
  static const char m_LogDir[];

  /// @brief Contains the string values of the settings names that will be saved
  static const char *const m_SettingsName[];

  /// @brief Core config filename (not the Qt settings filename)
  static const char m_ConfigFilename[];

signals:
  void tlsChanged();
  void screenNameChanged();
  void invertConnectionChanged();
};

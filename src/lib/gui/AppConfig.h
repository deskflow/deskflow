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

#include "CommonConfig.h"
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
#include <mutex>
#include <qvariant.h>

class QSettings;
class SettingsDialog;
class ServerConfig;
class LicenseHandler;
class ActivationDialog;

enum class ProcessMode { kService, kDesktop };

const ElevateMode kDefaultElevateMode = ElevateAsNeeded;
const QString kDefaultLogFile = "synergy.log";

#if defined(Q_OS_WIN)
const ProcessMode kDefaultProcessMode = ProcessMode::kService;
#else
const ProcessMode kDefaultProcessMode = ProcessMode::kDesktop;
#endif

/**
 * @brief Simply reads and writes app settings.
 *
 * Important: Maintain a clear separation of concerns and keep it simple.
 * It is tempting to add logic (e.g. license checks) to this class since it
 * instance is widely accessible, but that has previously led to this class
 * becoming a god object.
 */
class AppConfig : public QObject,
                  public synergy::gui::CommonConfig,
                  public synergy::gui::IAppConfig {
  Q_OBJECT

  friend class SettingsDialog;
  friend class MainWindow;
  friend class SetupWizard;
  friend class ServerConfig;
  friend class ActivationDialog;

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
    kServiceEnabled = 37,
    kCloseToTray = 38,
    kMainWindowSize = 39,
    kMainWindowPosition = 40,
  };

public:
  explicit AppConfig();

  /// @brief Underlying configuration reader/writer
  synergy::gui::ConfigScopes &config();

  /// @brief Saves the setting to the current scope
  void saveSettings() override;

  /**
   * Getters
   */

  bool isWritable() const;
  bool isSystemScoped() const;
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
  bool serviceEnabled() const;
  bool closeToTray() const;
  QString serialKey() const;
  bool activationHasRun() const;
  bool tlsEnabled() const override;
  QString tlsCertPath() const override;
  QString tlsKeyLength() const override;
  std::optional<QSize> mainWindowSize() const;
  std::optional<QPoint> mainWindowPosition() const;

private:
  /// @brief Loads the setting from the current scope
  void extracted();
  void loadSettings() override;
  void loadSerialKey();
  void loadElevateMode();

  static QString settingName(AppConfig::Setting name);

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
  void setServiceEnabled(bool enabled);
  void setCloseToTray(bool minimize);
  void setActivationHasRun(bool value);
  void setTlsCertPath(const QString &path);
  void setTlsKeyLength(const QString &length);
  void setInvertConnection(bool value);
  void setMainWindowSize(const QSize &size);
  void setMainWindowPosition(const QPoint &position);
  void loadCommonSettings();
  void loadScopeSettings();

  /**
   * @brief Loads a setting if it exists, otherwise returns `std::nullopt`
   *
   * @param toType A function to convert the QVariant to the desired type.
   */
  template <typename T>
  std::optional<T>
  loadOptional(Setting name, std::function<T(QVariant)> toType) const;

  template <typename T>
  void setOptional(Setting name, const std::optional<T> &value);

  /// @brief Sets the user preference to load from SystemScope.
  /// @param [in] value
  ///             True - This will set the variable and load the global scope
  ///             settings. False - This will set the variable and load the user
  ///             scope settings.
  void setLoadFromSystemScope(bool value);

  /// @brief Loads config from the underlying reader/writer
  void loadAllScopes();

  /// @brief Sets the value of a setting
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved
  template <typename T> void setSetting(AppConfig::Setting name, T value);

  /// @brief Sets the value of a common setting
  /// which should have the same value for all scopes
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved
  template <typename T> void setCommonSetting(AppConfig::Setting name, T value);

  /// @brief Loads a setting
  /// @param [in] name The setting to be loaded
  /// @param [in] defaultValue The default value of the setting
  QVariant loadSetting(
      AppConfig::Setting name, const QVariant &defaultValue = QVariant());

  /// @brief Loads a common setting
  /// @param [in] name The setting to be loaded
  /// @param [in] defaultValue The default value of the setting
  QVariant loadCommonSetting(
      AppConfig::Setting name, const QVariant &defaultValue = QVariant()) const;

  /// @brief Sets the setting in the config checking if it has changed and
  /// flagging that settings
  ///         needs to be saved if the setting was different
  /// @param [in] variable the setting that will be changed
  /// @param [in] newValue The new value of the setting
  template <typename T> void setSettingModified(T &variable, const T &newValue);

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

  synergy::gui::ConfigScopes m_Config;
  CoreInterface m_CoreInterface;
  QString m_ScreenName = QHostInfo::localHostName();
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
  bool m_ServiceEnabled = kDefaultProcessMode == ProcessMode::kService;
  bool m_CloseToTray = false;
  QString m_TlsCertPath = defaultTlsCertPath();
  QString m_TlsKeyLength = "2048";
  std::optional<QSize> m_MainWindowSize;
  std::optional<QPoint> m_MainWindowPosition;

  /**
   * @brief Flag is set when any TLS is setting is changed, and is reset
   * when the TLS changed event is emitted.
   */
  bool m_TlsChanged = false;

  /// @brief should the setting be loaded from
  /// SystemScope
  ///         If the user has settings but this is
  ///         true then system settings will be
  ///         loaded instead of the users
  bool m_LoadFromSystemScope = false;

  /// @brief As the settings will be accessible by multiple objects this lock
  /// will ensure that
  ///         it cant be modified by more that one object at a time if the
  ///         setting is being switched from system to user.
  std::mutex m_settings_lock;

  static const char m_CoreServerName[];
  static const char m_CoreClientName[];
  static const char m_LogDir[];

  /// @brief Contains the string values of the settings names that will be saved
  static const char *const m_SettingsName[];

  /// @brief Contains the name of the default configuration filename
  static const char m_ConfigFilename[];

signals:
  void loaded();
  void saved();
  void tlsChanged();
  void screenNameChanged();
  void invertConnectionChanged();
};

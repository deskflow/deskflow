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
#include "Config.h"
#include "CoreInterface.h"
#include "LicenseManager.h"
#include "gui/ElevateMode.h"
#include "shared/EditionType.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <mutex>

class QSettings;
class SettingsDialog;
class LicenseRegister;
class ServerConfig;
class LicenseManager;
class ActivationDialog;
class LicenseRegistry;

enum class ProcessMode { kService, kDesktop };

const ElevateMode kDefaultElevateMode = ElevateAsNeeded;

#if defined(Q_OS_WIN)
const ProcessMode kDefaultProcessMode = ProcessMode::kService;
#else
const ProcessMode kDefaultProcessMode = ProcessMode::kDesktop;
#endif

/// @brief Reads and writes application specific settings
class AppConfig : public QObject, public synergy::gui::CommonConfig {
  Q_OBJECT

  friend class SettingsDialog;
  friend class MainWindow;
  friend class SetupWizard;
  friend class ServerConfig;
  friend class LicenseManager;
  friend class ActivationDialog;
  friend class LicenseRegistry;

public:
  explicit AppConfig();

  /// @brief Underlying configuration reader/writer
  synergy::gui::Config &config();

  void saveSettings() override;
  void applyAppSettings() const;

  /// @brief Generates TLS certificate
  /// @param [in] bool forceGeneration Generate certificate even if it's exists.
  void generateCertificate(bool forceGeneration = false) const;

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
  ElevateMode elevateMode();
  bool cryptoAvailable() const;
  bool cryptoEnabled() const;
  bool autoHide();
  bool invertScrollDirection() const;
  unsigned long long licenseNextCheck() const;
  const QString &guid() const;
  bool languageSync() const;
  bool preventSleep() const;
  bool clientHostMode() const;
  bool serverClientMode() const;
  bool initiateConnectionFromServer() const;
  bool serverGroupChecked() const;
  bool useExternalConfig() const;
  const QString &configFile() const;
  bool useInternalConfig() const;
  bool clientGroupChecked() const;
  QString serverHostname() const;
  QString lastVersion() const;
  bool serviceEnabled() const;
  bool minimizeToTray();
  bool minimizeOnClose() const;

  /// @brief Gets the current TLS certificate path
  /// @return QString The path to the cert
  QString tlsCertPath() const;

  /// @brief Get the key length to be used for the private key of a TLS cert
  /// @return QString The key length in bits
  QString tlsKeyLength() const;

#ifdef SYNERGY_ENABLE_LICENSING
  Edition edition() const;
  QString serialKey() const;
  int lastExpiringWarningTime() const;
  bool activationHasRun() const;
#endif

protected:
  enum class Setting {
    kScreenName,
    kPort,
    kInterfaceSetting,
    kLogLevel,
    kLogToFile,
    kLogFilename,
    kWizardLastRun,
    kStartedBefore,
    kElevateModeSetting,
    kElevateModeEnum,
    kEditionSetting,
    kCryptoEnabled,
    kAutoHide,
    kSerialKey,
    kLastVersion,
    kLastExpireWarningTime,
    kActivationHasRun,
    kMinimizeToTray,
    kActivateEmail,
    kLoadSystemSettings,
    kGroupServerCheck,
    kUseExternalConfig,
    kConfigFile,
    kUseInternalConfig,
    kGroupClientCheck,
    kServerHostname,
    kTlsCertPath,
    kTlsKeyLength,
    kPreventSleep,
    kLanguageSync,
    kInvertScrollDirection,
    kGuid,
    kLicenseRegistryUrl,
    kLicenseNextCheck,
    kInitiateConnectionFromServer,
    kClientHostMode,
    kServerClientMode,
    kServiceEnabled,
    kMinimizeOnClose
  };

  static QString settingName(AppConfig::Setting name);

  /// @brief Loads the setting from the current scope
  void loadSettings() override;

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
  void setCryptoEnabled(bool e);
  void setEdition(Edition);
  void setSerialKey(const QString &serial);
  void clearSerialKey();
  void setLastExpiringWarningTime(int t);
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
  void setClientHostMode(bool newValue);
  void setServerClientMode(bool newValue);
  AppConfig &activationHasRun(bool value);
  void setMinimizeToTray(bool b);
  void setLastVersion(const QString &version);
  void setServiceEnabled(bool enabled);
  void setMinimizeOnClose(bool minimize);

  /// @brief Sets the user preference to load from SystemScope.
  /// @param [in] value
  ///             True - This will set the variable and load the global scope
  ///             settings. False - This will set the variable and load the user
  ///             scope settings.
  void setLoadFromSystemScope(bool value);

  /// @brief Set the path to the TLS/SSL certificate file that will be used
  /// @param [in] path The path to the Certificate
  void setTlsCertPath(const QString &path);

  /// @brief Sets the key length of the private key to use in a TLS connection
  /// @param [in] QString length The key length eg: 1024, 2048, 4096
  void setTlsKeyLength(const QString &length);

private:
  /// @brief Loads config from the underlying reader/writer
  void load();

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
  void loadScope(synergy::gui::Config::Scope scope);

  /// @brief This function sets default values
  /// for settings that shouldn't be copied from between scopes.
  void setDefaultValues();

  synergy::gui::Config m_Config;
  CoreInterface m_CoreInterface;
  QString m_ScreenName = "";
  int m_Port = 24800;
  QString m_Interface = "";
  int m_LogLevel = 0;
  bool m_LogToFile = false;
  QString m_LogFilename = "";
  int m_WizardLastRun = 0;
  bool m_StartedBefore = false;
  ElevateMode m_ElevateMode = kDefaultElevateMode;
  Edition m_Edition = Edition::kUnregistered;
  QString m_ActivateEmail = "";
  bool m_CryptoEnabled = false;
  bool m_AutoHide = false;
  QString m_Serialkey = "";
  QString m_LastVersion = "";
  QString m_Guid = "";
  unsigned long long m_licenseNextCheck = 0;
  int m_LastExpiringWarningTime = 0;
  bool m_ActivationHasRun = false;
  bool m_MinimizeToTray = true;
  bool m_InvertScrollDirection = false;
  bool m_LanguageSync = true;
  bool m_PreventSleep = false;
  bool m_InitiateConnectionFromServer = false;
  bool m_ClientHostMode = true;
  bool m_ServerClientMode = true;
  bool m_ServerGroupChecked = false;
  bool m_UseExternalConfig = false;
  QString m_ConfigFile = "";
  bool m_UseInternalConfig = false;
  bool m_ClientGroupChecked = false;
  QString m_ServerHostname = "";
  bool m_ServiceEnabled = kDefaultProcessMode == ProcessMode::kService;
  bool m_MinimizeOnClose = true;

  /// @brief The path to the TLS certificate file
  QString m_TlsCertPath = "";

  /// @brief The key length of the TLS cert to make
  QString m_TlsKeyLength = "";

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
  void sslToggled() const;
  void screenNameChanged() const;
};

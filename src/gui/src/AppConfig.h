/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#if !defined(APPCONFIG_H)

#define APPCONFIG_H

#include <QObject>
#include <QString>
#include <QVariant>
#include "ElevateMode.h"
#include <shared/EditionType.h>
#include <mutex>
#include "ConfigBase.h"
#include "CoreInterface.h"

// this should be incremented each time a new page is added. this is
// saved to settings when the user finishes running the wizard. if
// the saved wizard version is lower than this number, the wizard
// will be displayed. each version incrememnt should be described
// here...
//
//   1: first version
//   2: added language page
//   3: added premium page and removed
//   4: ssl plugin 'ns' v1.0
//   5: ssl plugin 'ns' v1.1
//   6: ssl plugin 'ns' v1.2
//   7: serial key activation
//   8: Visual Studio 2015 support
//
const int kWizardVersion = 8;

class QSettings;
class SettingsDialog;

enum ProcessMode {
    Service,
    Desktop
};

class AppConfig: public QObject, public GUI::Config::ConfigBase
{
    Q_OBJECT

    friend class SettingsDialog;
    friend class MainWindow;
    friend class SetupWizard;

    public:
        AppConfig();
        ~AppConfig() override;

    public:

        bool isSystemScoped() const;

        const QString& screenName() const;
        int port() const;
        const QString& networkInterface() const;
        int logLevel() const;
        bool logToFile() const;
        const QString& logFilename() const;
        const QString logFilenameCmd() const;
        QString logLevelText() const;
        ProcessMode processMode() const;
        bool wizardShouldRun() const;
        const QString& language() const;
        bool startedBefore() const;
        bool autoConfig() const;
        void setAutoConfig(bool autoConfig);
        QString autoConfigServer() const;
        void setAutoConfigServer(const QString& autoConfigServer);
#ifndef SYNERGY_ENTERPRISE
        void setEdition(Edition);
        Edition edition() const;
        void setSerialKey(const QString& serial);
        void clearSerialKey();
        QString serialKey();
        int lastExpiringWarningTime() const;
        void setLastExpiringWarningTime(int t);
#endif

        QString synergysName() const;
        QString synergycName() const;
        QString synergyProgramDir() const;
        QString synergyLogDir() const;

        void persistLogDir();
        ElevateMode elevateMode();

        bool isCryptoAvailable() const;
        void setCryptoEnabled(bool e);
        bool getCryptoEnabled() const;

        void setAutoHide(bool b);
        bool getAutoHide();
#ifndef SYNERGY_ENTERPRISE
        bool activationHasRun() const;
        AppConfig& activationHasRun(bool value);
#endif
        /// @brief Sets the user preference to load from SystemScope.
        /// @param [in] value
        ///             True - This will set the variable, and save the user settings before loading the global scope settings
        ///             False - This will load the UserScope then set the variable and save.
        void setLoadFromSystemScope(bool value);


        bool    getServerGroupChecked() const;
        bool    getUseExternalConfig() const;
        QString getConfigFile() const;
        bool    getUseInternalConfig() const;
        bool    getClientGroupChecked() const;
        QString getServerHostname() const;

        /// @brief Gets the current TLS certificate path
        /// @return QString The path to the cert
        QString getTLSCertPath() const;

        /// @brief Get the key length to be used for the private key of a TLS cert
        /// @return QString The key length in bits
        QString     getTLSKeyLength() const;

        void setServerGroupChecked(bool);
        void setUseExternalConfig(bool) ;
        void setConfigFile(const QString&);
        void setUseInternalConfig(bool) ;
        void setClientGroupChecked(bool) ;
        void setServerHostname(const QString&);

        /// @brief Set the path to the TLS/SSL certificate file that will be used
        /// @param [in] path The path to the Certificate
        void setTLSCertPath(const QString& path);

        /// @brief Sets the key length of the private key to use in a TLS connection
        /// @param [in] QString length The key length eg: 1024, 2048, 4096
        void setTLSKeyLength(const QString& length);


        QString lastVersion() const;

        void setMinimizeToTray(bool b);
        bool getMinimizeToTray();

        void saveSettings() override;
        void setLastVersion(const QString& version);

        /// @brief Generates TLS certificate
        /// @param [in] bool forceGeneration Generate certificate even if it's exists.
        void generateCertificate(bool forceGeneration=false) const;

protected:
    /// @brief The enumeration to easily access the names of the setting inside m_SynergySettingsName
    enum Setting {
        kScreenName,
        kPort,
        kInterfaceSetting,
        kLogLevel,
        kLogToFile,
        kLogFilename,
        kWizardLastRun,
        kLanguage,
        kStartedBefore,
        kAutoConfig,
        kAutoConfigServer,
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
        kTLSCertPath,
        kTLSKeyLength,
    };

        void setScreenName(const QString& s);
        void setPort(int i);
        void setNetworkInterface(const QString& s);
        void setLogLevel(int i);
        void setLogToFile(bool b);
        void setLogFilename(const QString& s);
        void setWizardHasRun();
        void setLanguage(const QString& language);
        void setStartedBefore(bool b);
        void setElevateMode(ElevateMode em);

        /// @brief loads the setting from the current scope
        /// @param ignoreSystem should the load feature ignore the globalScope setting that was saved
        void loadSettings() override;
        static QString settingName(AppConfig::Setting name);

    private:

        QString m_ScreenName;
        int m_Port;
        QString m_Interface;
        int m_LogLevel;
        bool m_LogToFile;
        QString m_LogFilename;
        int m_WizardLastRun;
        ProcessMode m_ProcessMode;
        QString m_Language;
        bool m_StartedBefore;
        bool m_AutoConfig;
        QString m_AutoConfigServer;
        ElevateMode m_ElevateMode;
        Edition m_Edition;
        QString m_ActivateEmail;
        bool m_CryptoEnabled;
        bool m_AutoHide;
        QString m_Serialkey;
        QString m_lastVersion;
        int m_LastExpiringWarningTime;
        bool m_ActivationHasRun;
        bool m_MinimizeToTray;

        bool m_ServerGroupChecked;
        bool m_UseExternalConfig;
        QString m_ConfigFile;
        bool m_UseInternalConfig;
        bool m_ClientGroupChecked;
        QString m_ServerHostname;

        QString m_TLSCertificatePath;   /// @brief The path to the TLS certificate file
        QString m_TLSKeyLength;         /// @brief The key length of the TLS cert to make

        bool m_LoadFromSystemScope;     /// @brief should the setting be loaded from SystemScope
                                        ///         If the user has settings but this is true then
                                        ///         system settings will be loaded instead of the users

        CoreInterface m_CoreInterface;

        static const char m_SynergysName[];
        static const char m_SynergycName[];
        static const char m_SynergyLogDir[];

        /// @brief Contains the string values of the settings names that will be saved
        static const char* m_SynergySettingsName[];

        /// @brief Contains the name of the default configuration filename
        static const char synergyConfigName[];

        /// @brief Sets the value of a setting
        /// @param [in] name The Setting to be saved
        /// @param [in] value The Value to be saved
        template <typename T>
        void setSetting(AppConfig::Setting name, T value);

        /// @brief Loads a setting
        /// @param [in] name The setting to be loaded
        /// @param [in] defaultValue The default value of the setting
        QVariant loadSetting(AppConfig::Setting name, const QVariant& defaultValue = QVariant());

        /// @brief As the settings will be accessible by multiple objects this lock will ensure that
        ///         it cant be modified by more that one object at a time if the setting is being switched
        ///         from system to user.
        std::mutex m_settings_lock;

        /// @brief Sets the setting in the config checking if it has changed and flagging that settings
        ///         needs to be saved if the setting was different
        /// @param [in] variable the setting that will be changed
        /// @param [in] newValue The new value of the setting
        template <typename T>
        void setSettingModified(T& variable,const T& newValue);

    signals:
        void sslToggled() const;
        void zeroConfToggled();
};

#endif

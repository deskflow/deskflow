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
#include "ElevateMode.h"
#include <shared/EditionType.h>

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

enum ProcessMode { Service, Desktop };

class AppConfig : public QObject {
    Q_OBJECT

    friend class SettingsDialog;
    friend class MainWindow;
    friend class SetupWizard;

public:
    AppConfig (QSettings* settings);
    ~AppConfig ();

public:
    const QString& screenName () const;
    int port () const;
    const QString& networkInterface () const;
    int logLevel () const;
    bool logToFile () const;
    const QString& logFilename () const;
    const QString logFilenameCmd () const;
    QString logLevelText () const;
    ProcessMode processMode () const;
    bool wizardShouldRun () const;
    const QString& language () const;
    bool startedBefore () const;
    bool autoConfig () const;
    void setAutoConfig (bool autoConfig);
    bool autoConfigPrompted ();
    void setAutoConfigPrompted (bool prompted);
    void setEdition (Edition);
    Edition edition () const;
    QString setSerialKey (QString serial);
    void clearSerialKey ();
    QString serialKey ();
    int lastExpiringWarningTime () const;
    void setLastExpiringWarningTime (int t);

    QString synergysName () const;
    QString synergycName () const;
    QString synergyProgramDir () const;
    QString synergyLogDir () const;

    bool detectPath (const QString& name, QString& path);
    void persistLogDir ();
    ElevateMode elevateMode ();

    void setCryptoEnabled (bool e);
    bool getCryptoEnabled () const;

    void setAutoHide (bool b);
    bool getAutoHide ();

    bool activationHasRun () const;
    AppConfig& activationHasRun (bool value);

    QString lastVersion () const;

    void saveSettings ();
    void setLastVersion (QString version);

protected:
    QSettings& settings ();
    void setScreenName (const QString& s);
    void setPort (int i);
    void setNetworkInterface (const QString& s);
    void setLogLevel (int i);
    void setLogToFile (bool b);
    void setLogFilename (const QString& s);
    void setWizardHasRun ();
    void setLanguage (const QString language);
    void setStartedBefore (bool b);
    void setElevateMode (ElevateMode em);
    void loadSettings ();

private:
    QSettings* m_pSettings;
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
    ElevateMode m_ElevateMode;
    bool m_AutoConfigPrompted;
    Edition m_Edition;
    QString m_ActivateEmail;
    bool m_CryptoEnabled;
    bool m_AutoHide;
    QString m_Serialkey;
    QString m_lastVersion;
    int m_LastExpiringWarningTime;
    bool m_ActivationHasRun;

    static const char m_SynergysName[];
    static const char m_SynergycName[];
    static const char m_SynergyLogDir[];

signals:
    void sslToggled (bool enabled);
};

#endif

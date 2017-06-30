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

#include "AppConfig.h"
#include "QUtility.h"

#include <QtCore>
#include <QtNetwork>

#if defined(Q_OS_WIN)
const char AppConfig::m_SynergysName[]  = "synergys.exe";
const char AppConfig::m_SynergycName[]  = "synergyc.exe";
const char AppConfig::m_SynergyLogDir[] = "log/";
#define DEFAULT_PROCESS_MODE Service
#else
const char AppConfig::m_SynergysName[]  = "synergys";
const char AppConfig::m_SynergycName[]  = "synergyc";
const char AppConfig::m_SynergyLogDir[] = "/var/log/";
#define DEFAULT_PROCESS_MODE Desktop
#endif

const ElevateMode defaultElevateMode = ElevateAsNeeded;

static const char* logLevelNames[] = {
    "ERROR", "WARNING", "NOTE", "INFO", "DEBUG", "DEBUG1", "DEBUG2"};

AppConfig::AppConfig (QSettings* settings)
    : m_pSettings (settings),
      m_ScreenName (),
      m_Port (24800),
      m_Interface (),
      m_LogLevel (0),
      m_WizardLastRun (0),
      m_ProcessMode (DEFAULT_PROCESS_MODE),
      m_AutoConfig (true),
      m_ElevateMode (defaultElevateMode),
      m_AutoConfigPrompted (false),
      m_CryptoEnabled (false),
      m_AutoHide (false),
      m_LastExpiringWarningTime (0) {
    Q_ASSERT (m_pSettings);

    loadSettings ();
}

AppConfig::~AppConfig () {
    saveSettings ();
}

const QString&
AppConfig::screenName () const {
    return m_ScreenName;
}

int
AppConfig::port () const {
    return m_Port;
}

const QString&
AppConfig::networkInterface () const {
    return m_Interface;
}

int
AppConfig::logLevel () const {
    return m_LogLevel;
}

bool
AppConfig::logToFile () const {
    return m_LogToFile;
}

const QString&
AppConfig::logFilename () const {
    return m_LogFilename;
}

QString
AppConfig::synergyLogDir () const {
#if defined(Q_OS_WIN)
    // on windows, we want to log to program files
    return synergyProgramDir () + "log/";
#else
    // on unix, we'll log to the standard log dir
    return "/var/log/";
#endif
}

QString
AppConfig::synergyProgramDir () const {
    // synergy binaries should be in the same dir.
    return QCoreApplication::applicationDirPath () + "/";
}

void
AppConfig::persistLogDir () {
    QDir dir = synergyLogDir ();

    // persist the log directory
    if (!dir.exists ()) {
        dir.mkpath (dir.path ());
    }
}

const QString
AppConfig::logFilenameCmd () const {
    QString filename = m_LogFilename;
#if defined(Q_OS_WIN)
    // wrap in quotes in case username contains spaces.
    filename = QString ("\"%1\"").arg (filename);
#endif
    return filename;
}

QString
AppConfig::logLevelText () const {
    return logLevelNames[logLevel ()];
}

ProcessMode
AppConfig::processMode () const {
    return m_ProcessMode;
}

bool
AppConfig::wizardShouldRun () const {
    return m_WizardLastRun < kWizardVersion;
}

const QString&
AppConfig::language () const {
    return m_Language;
}

bool
AppConfig::startedBefore () const {
    return m_StartedBefore;
}

bool
AppConfig::autoConfig () const {
    return m_AutoConfig;
}

void
AppConfig::loadSettings () {
    m_ScreenName = settings ()
                       .value ("screenName", QHostInfo::localHostName ())
                       .toString ();
    m_Port        = settings ().value ("port", 24800).toInt ();
    m_Interface   = settings ().value ("interface").toString ();
    m_LogLevel    = settings ().value ("logLevel", 3).toInt (); // level 3: INFO
    m_LogToFile   = settings ().value ("logToFile", false).toBool ();
    m_LogFilename = settings ()
                        .value ("logFilename", synergyLogDir () + "synergy.log")
                        .toString ();
    m_WizardLastRun = settings ().value ("wizardLastRun", 0).toInt ();
    m_Language =
        settings ().value ("language", QLocale::system ().name ()).toString ();
    m_StartedBefore      = settings ().value ("startedBefore", false).toBool ();
    m_AutoConfig         = settings ().value ("autoConfig", true).toBool ();
    QVariant elevateMode = settings ().value ("elevateModeEnum");
    if (!elevateMode.isValid ()) {
        elevateMode = settings ().value (
            "elevateMode", QVariant (static_cast<int> (defaultElevateMode)));
    }
    m_ElevateMode = static_cast<ElevateMode> (elevateMode.toInt ());
    m_AutoConfigPrompted =
        settings ().value ("autoConfigPrompted", false).toBool ();
    m_Edition = static_cast<Edition> (
        settings ().value ("edition", kUnregistered).toInt ());
    m_ActivateEmail = settings ().value ("activateEmail", "").toString ();
    m_CryptoEnabled = settings ().value ("cryptoEnabled", true).toBool ();
    m_AutoHide      = settings ().value ("autoHide", false).toBool ();
    m_Serialkey   = settings ().value ("serialKey", "").toString ().trimmed ();
    m_lastVersion = settings ().value ("lastVersion", "Unknown").toString ();
    m_LastExpiringWarningTime =
        settings ().value ("lastExpiringWarningTime", 0).toInt ();
    m_ActivationHasRun =
        settings ().value ("activationHasRun", false).toBool ();
}

void
AppConfig::saveSettings () {
    settings ().setValue ("screenName", m_ScreenName);
    settings ().setValue ("port", m_Port);
    settings ().setValue ("interface", m_Interface);
    settings ().setValue ("logLevel", m_LogLevel);
    settings ().setValue ("logToFile", m_LogToFile);
    settings ().setValue ("logFilename", m_LogFilename);
    settings ().setValue ("wizardLastRun", kWizardVersion);
    settings ().setValue ("language", m_Language);
    settings ().setValue ("startedBefore", m_StartedBefore);
    settings ().setValue ("autoConfig", m_AutoConfig);
    // Refer to enum ElevateMode declaration for insight in to why this
    // flag is mapped this way
    settings ().setValue ("elevateMode", m_ElevateMode == ElevateAlways);
    settings ().setValue ("elevateModeEnum", static_cast<int> (m_ElevateMode));
    settings ().setValue ("autoConfigPrompted", m_AutoConfigPrompted);
    settings ().setValue ("edition", m_Edition);
    settings ().setValue ("cryptoEnabled", m_CryptoEnabled);
    settings ().setValue ("autoHide", m_AutoHide);
    settings ().setValue ("serialKey", m_Serialkey);
    settings ().setValue ("lastVersion", m_lastVersion);
    settings ().setValue ("lastExpiringWarningTime", m_LastExpiringWarningTime);
    settings ().setValue ("activationHasRun", m_ActivationHasRun);
    settings ().sync ();
}

bool
AppConfig::activationHasRun () const {
    return m_ActivationHasRun;
}

AppConfig&
AppConfig::activationHasRun (bool value) {
    m_ActivationHasRun = value;
    return *this;
}

QString
AppConfig::lastVersion () const {
    return m_lastVersion;
}

void
AppConfig::setLastVersion (QString version) {
    m_lastVersion = version;
}

QSettings&
AppConfig::settings () {
    return *m_pSettings;
}

void
AppConfig::setScreenName (const QString& s) {
    m_ScreenName = s;
}

void
AppConfig::setPort (int i) {
    m_Port = i;
}

void
AppConfig::setNetworkInterface (const QString& s) {
    m_Interface = s;
}

void
AppConfig::setLogLevel (int i) {
    m_LogLevel = i;
}

void
AppConfig::setLogToFile (bool b) {
    m_LogToFile = b;
}

void
AppConfig::setLogFilename (const QString& s) {
    m_LogFilename = s;
}

void
AppConfig::setWizardHasRun () {
    m_WizardLastRun = kWizardVersion;
}

void
AppConfig::setLanguage (const QString language) {
    m_Language = language;
}

void
AppConfig::setStartedBefore (bool b) {
    m_StartedBefore = b;
}

void
AppConfig::setElevateMode (ElevateMode em) {
    m_ElevateMode = em;
}

void
AppConfig::setAutoConfig (bool autoConfig) {
    m_AutoConfig = autoConfig;
}

bool
AppConfig::autoConfigPrompted () {
    return m_AutoConfigPrompted;
}

void
AppConfig::setAutoConfigPrompted (bool prompted) {
    m_AutoConfigPrompted = prompted;
}

void
AppConfig::setEdition (Edition e) {
    m_Edition = e;
}

Edition
AppConfig::edition () const {
    return m_Edition;
}

QString
AppConfig::setSerialKey (QString serial) {
    using std::swap;
    swap (serial, m_Serialkey);
    return serial;
}

void
AppConfig::clearSerialKey () {
    m_Serialkey.clear ();
}

QString
AppConfig::serialKey () {
    return m_Serialkey;
}

int
AppConfig::lastExpiringWarningTime () const {
    return m_LastExpiringWarningTime;
}

void
AppConfig::setLastExpiringWarningTime (int t) {
    m_LastExpiringWarningTime = t;
}

QString
AppConfig::synergysName () const {
    return m_SynergysName;
}

QString
AppConfig::synergycName () const {
    return m_SynergycName;
}

ElevateMode
AppConfig::elevateMode () {
    return m_ElevateMode;
}

void
AppConfig::setCryptoEnabled (bool e) {
    m_CryptoEnabled = e;
    emit sslToggled (e);
}

bool
AppConfig::getCryptoEnabled () const {
    return (edition () == kPro) && m_CryptoEnabled;
}

void
AppConfig::setAutoHide (bool b) {
    m_AutoHide = b;
}

bool
AppConfig::getAutoHide () {
    return m_AutoHide;
}

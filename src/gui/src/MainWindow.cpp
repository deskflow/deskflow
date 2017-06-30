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

#define DOWNLOAD_URL "http://symless.com/?source=gui"

#include <iostream>

#include "MainWindow.h"

#include "Fingerprint.h"
#include "AboutDialog.h"
#include "ServerConfigDialog.h"
#include "SettingsDialog.h"
#include "ActivationDialog.h"
#include "ZeroconfService.h"
#include "DataDownloader.h"
#include "CommandProcess.h"
#include "LicenseManager.h"
#include <shared/EditionType.h>
#include "QUtility.h"
#include "ProcessorArch.h"
#include "SslCertificate.h"

#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include <QNetworkAccessManager>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDesktopWidget>

#if defined(Q_OS_MAC)
#include <ApplicationServices/ApplicationServices.h>
#endif

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#if defined(Q_OS_WIN)
static const char synergyConfigName[] = "synergy.sgc";
static const QString synergyConfigFilter (
    QObject::tr ("Synergy Configurations (*.sgc);;All files (*.*)"));
static QString bonjourBaseUrl         = "http://binaries.symless.com/bonjour/";
static const char bonjourFilename32[] = "Bonjour.msi";
static const char bonjourFilename64[] = "Bonjour64.msi";
static const char bonjourTargetFilename[] = "Bonjour.msi";
#else
static const char synergyConfigName[] = "synergy.conf";
static const QString synergyConfigFilter (
    QObject::tr ("Synergy Configurations (*.conf);;All files (*.*)"));
#endif

static const char* synergyIconFiles[] = {
    ":/res/icons/16x16/synergy-disconnected.png",
    ":/res/icons/16x16/synergy-disconnected.png",
    ":/res/icons/16x16/synergy-connected.png",
    ":/res/icons/16x16/synergy-transfering.png"};

MainWindow::MainWindow (QSettings& settings, AppConfig& appConfig,
                        LicenseManager& licenseManager)
    : m_Settings (settings),
      m_AppConfig (&appConfig),
      m_LicenseManager (&licenseManager),
      m_pSynergy (NULL),
      m_SynergyState (synergyDisconnected),
      m_ServerConfig (&m_Settings, 5, 3, m_AppConfig->screenName (), this),
      m_pTempConfigFile (NULL),
      m_pTrayIcon (NULL),
      m_pTrayIconMenu (NULL),
      m_AlreadyHidden (false),
      m_pMenuBar (NULL),
      m_pMenuFile (NULL),
      m_pMenuEdit (NULL),
      m_pMenuWindow (NULL),
      m_pMenuHelp (NULL),
      m_pZeroconfService (NULL),
      m_pDataDownloader (NULL),
      m_DownloadMessageBox (NULL),
      m_pCancelButton (NULL),
      m_SuppressAutoConfigWarning (false),
      m_BonjourInstall (NULL),
      m_SuppressEmptyServerWarning (false),
      m_ExpectedRunningState (kStopped),
      m_pSslCertificate (NULL),
      m_ActivationDialogRunning (false) {
    setupUi (this);

    createMenuBar ();
    loadSettings ();
    initConnections ();

    m_pWidgetUpdate->hide ();
    m_VersionChecker.setApp (appPath (appConfig.synergycName ()));
    m_pLabelScreenName->setText (getScreenName ());
    m_pLabelIpAddresses->setText (getIPAddresses ());

#if defined(Q_OS_WIN)
    // ipc must always be enabled, so that we can disable command when switching
    // to desktop mode.
    connect (&m_IpcClient,
             SIGNAL (readLogLine (const QString&)),
             this,
             SLOT (appendLogRaw (const QString&)));
    connect (&m_IpcClient,
             SIGNAL (errorMessage (const QString&)),
             this,
             SLOT (appendLogError (const QString&)));
    connect (&m_IpcClient,
             SIGNAL (infoMessage (const QString&)),
             this,
             SLOT (appendLogInfo (const QString&)));
    m_IpcClient.connectToHost ();
#endif

// change default size based on os
#if defined(Q_OS_MAC)
    resize (720, 550);
    setMinimumSize (size ());
#elif defined(Q_OS_LINUX)
    resize (700, 530);
    setMinimumSize (size ());
#endif

    m_SuppressAutoConfigWarning = true;
    m_pCheckBoxAutoConfig->setChecked (appConfig.autoConfig ());
    m_SuppressAutoConfigWarning = false;

    m_pComboServerList->hide ();
    m_pLabelPadlock->hide ();
    m_trialWidget->hide ();

    connect (this,
             SIGNAL (windowShown ()),
             this,
             SLOT (on_windowShown ()),
             Qt::QueuedConnection);

    connect (m_LicenseManager,
             SIGNAL (editionChanged (Edition)),
             this,
             SLOT (setEdition (Edition)),
             Qt::QueuedConnection);

    connect (m_LicenseManager,
             SIGNAL (beginTrial (bool)),
             this,
             SLOT (beginTrial (bool)),
             Qt::QueuedConnection);

    connect (m_LicenseManager,
             SIGNAL (endTrial (bool)),
             this,
             SLOT (endTrial (bool)),
             Qt::QueuedConnection);

    connect (m_AppConfig,
             SIGNAL (sslToggled (bool)),
             this,
             SLOT (sslToggled (bool)),
             Qt::QueuedConnection);

    setWindowTitle (m_LicenseManager->activeEditionName ());
    m_LicenseManager->refresh ();

    QString lastVersion    = m_AppConfig->lastVersion ();
    QString currentVersion = m_VersionChecker.getVersion ();
    if (lastVersion != currentVersion) {
        m_AppConfig->setLastVersion (currentVersion);
        m_AppConfig->saveSettings ();
        m_LicenseManager->notifyUpdate (lastVersion, currentVersion);
    }
}

MainWindow::~MainWindow () {
    if (appConfig ().processMode () == Desktop) {
        m_ExpectedRunningState = kStopped;
        stopDesktop ();
    }

    saveSettings ();

    delete m_pZeroconfService;

    if (m_DownloadMessageBox != NULL) {
        delete m_DownloadMessageBox;
    }

    if (m_BonjourInstall != NULL) {
        delete m_BonjourInstall;
    }

    delete m_pSslCertificate;
}

void
MainWindow::open () {
    createTrayIcon ();

    if (!autoHide ()) {
        showNormal ();
    }

    m_VersionChecker.checkLatest ();

    if (!appConfig ().autoConfigPrompted ()) {
        promptAutoConfig ();
    }

    // only start if user has previously started. this stops the gui from
    // auto hiding before the user has configured synergy (which of course
    // confuses first time users, who think synergy has crashed).
    if (appConfig ().startedBefore () &&
        appConfig ().processMode () == Desktop) {
        m_SuppressEmptyServerWarning = true;
        startSynergy ();
        m_SuppressEmptyServerWarning = false;
    }
}

void
MainWindow::onModeChanged (bool startDesktop, bool applyService) {
    if (appConfig ().processMode () == Service) {
        // ensure that the apply button actually does something, since desktop
        // mode screws around with connecting/disconnecting the action.
        disconnect (m_pButtonToggleStart,
                    SIGNAL (clicked ()),
                    m_pActionStartSynergy,
                    SLOT (trigger ()));
        connect (m_pButtonToggleStart,
                 SIGNAL (clicked ()),
                 m_pActionStartSynergy,
                 SLOT (trigger ()));

        if (applyService) {
            stopDesktop ();
            startSynergy ();
        }
    } else if ((appConfig ().processMode () == Desktop) && startDesktop) {
        stopService ();
        startSynergy ();
    }
}

void
MainWindow::setStatus (const QString& status) {
    m_pStatusLabel->setText (status);
}

void
MainWindow::createTrayIcon () {
    m_pTrayIconMenu = new QMenu (this);

    m_pTrayIconMenu->addAction (m_pActionStartSynergy);
    m_pTrayIconMenu->addAction (m_pActionStopSynergy);
    m_pTrayIconMenu->addSeparator ();

    m_pTrayIconMenu->addAction (m_pActionMinimize);
    m_pTrayIconMenu->addAction (m_pActionRestore);
    m_pTrayIconMenu->addSeparator ();
    m_pTrayIconMenu->addAction (m_pActionQuit);

    m_pTrayIcon = new QSystemTrayIcon (this);
    m_pTrayIcon->setContextMenu (m_pTrayIconMenu);

    connect (m_pTrayIcon,
             SIGNAL (activated (QSystemTrayIcon::ActivationReason)),
             this,
             SLOT (trayActivated (QSystemTrayIcon::ActivationReason)));

    setIcon (synergyDisconnected);

    m_pTrayIcon->show ();
}

void
MainWindow::retranslateMenuBar () {
    m_pMenuFile->setTitle (tr ("&File"));
    m_pMenuEdit->setTitle (tr ("&Edit"));
    m_pMenuWindow->setTitle (tr ("&Window"));
    m_pMenuHelp->setTitle (tr ("&Help"));
}

void
MainWindow::createMenuBar () {
    m_pMenuBar    = new QMenuBar (this);
    m_pMenuFile   = new QMenu ("", m_pMenuBar);
    m_pMenuEdit   = new QMenu ("", m_pMenuBar);
    m_pMenuWindow = new QMenu ("", m_pMenuBar);
    m_pMenuHelp   = new QMenu ("", m_pMenuBar);
    retranslateMenuBar ();

    m_pMenuBar->addAction (m_pMenuFile->menuAction ());
    m_pMenuBar->addAction (m_pMenuEdit->menuAction ());
#if !defined(Q_OS_MAC)
    m_pMenuBar->addAction (m_pMenuWindow->menuAction ());
#endif
    m_pMenuBar->addAction (m_pMenuHelp->menuAction ());

    m_pMenuFile->addAction (m_pActionStartSynergy);
    m_pMenuFile->addAction (m_pActionStopSynergy);
    m_pMenuFile->addSeparator ();
    m_pMenuFile->addAction (m_pActivate);
    m_pMenuFile->addSeparator ();
    m_pMenuFile->addAction (m_pActionSave);
    m_pMenuFile->addSeparator ();
    m_pMenuFile->addAction (m_pActionQuit);
    m_pMenuEdit->addAction (m_pActionSettings);
    m_pMenuWindow->addAction (m_pActionMinimize);
    m_pMenuWindow->addAction (m_pActionRestore);
    m_pMenuHelp->addAction (m_pActionAbout);

    setMenuBar (m_pMenuBar);
}

void
MainWindow::loadSettings () {
    // the next two must come BEFORE loading groupServerChecked and
    // groupClientChecked or
    // disabling and/or enabling the right widgets won't automatically work
    m_pRadioExternalConfig->setChecked (
        settings ().value ("useExternalConfig", false).toBool ());
    m_pRadioInternalConfig->setChecked (
        settings ().value ("useInternalConfig", true).toBool ());

    m_pGroupServer->setChecked (
        settings ().value ("groupServerChecked", false).toBool ());
    m_pLineEditConfigFile->setText (
        settings ()
            .value ("configFile", QDir::homePath () + "/" + synergyConfigName)
            .toString ());
    m_pGroupClient->setChecked (
        settings ().value ("groupClientChecked", true).toBool ());
    m_pLineEditHostname->setText (
        settings ().value ("serverHostname").toString ());
}

void
MainWindow::initConnections () {
    connect (m_pActionMinimize, SIGNAL (triggered ()), this, SLOT (hide ()));
    connect (
        m_pActionRestore, SIGNAL (triggered ()), this, SLOT (showNormal ()));
    connect (m_pActionStartSynergy,
             SIGNAL (triggered ()),
             this,
             SLOT (startSynergy ()));
    connect (m_pActionStopSynergy,
             SIGNAL (triggered ()),
             this,
             SLOT (stopSynergy ()));
    connect (m_pActionQuit, SIGNAL (triggered ()), qApp, SLOT (quit ()));
    connect (&m_VersionChecker,
             SIGNAL (updateFound (const QString&)),
             this,
             SLOT (updateFound (const QString&)));
}

void
MainWindow::saveSettings () {
    // program settings
    settings ().setValue ("groupServerChecked", m_pGroupServer->isChecked ());
    settings ().setValue ("useExternalConfig",
                          m_pRadioExternalConfig->isChecked ());
    settings ().setValue ("configFile", m_pLineEditConfigFile->text ());
    settings ().setValue ("useInternalConfig",
                          m_pRadioInternalConfig->isChecked ());
    settings ().setValue ("groupClientChecked", m_pGroupClient->isChecked ());
    settings ().setValue ("serverHostname", m_pLineEditHostname->text ());

    settings ().sync ();
}

void
MainWindow::setIcon (qSynergyState state) {
    QIcon icon;
    icon.addFile (synergyIconFiles[state]);

    if (m_pTrayIcon)
        m_pTrayIcon->setIcon (icon);
}

void
MainWindow::trayActivated (QSystemTrayIcon::ActivationReason reason) {
#ifndef Q_OS_WIN
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible ()) {
            hide ();
        } else {
            showNormal ();
            activateWindow ();
        }
    }
#endif
}

void
MainWindow::logOutput () {
    if (m_pSynergy) {
        QString text (m_pSynergy->readAllStandardOutput ());
        foreach (QString line, text.split (QRegExp ("\r|\n|\r\n"))) {
            if (!line.isEmpty ()) {
                appendLogRaw (line);
            }
        }
    }
}

void
MainWindow::logError () {
    if (m_pSynergy) {
        appendLogRaw (m_pSynergy->readAllStandardError ());
    }
}

void
MainWindow::updateFound (const QString& version) {
    m_pWidgetUpdate->show ();
    m_pLabelUpdate->setText (tr ("<p>Your version of Synergy is out of date. "
                                 "Version <b>%1</b> is now available to "
                                 "<a href=\"%2\">download</a>.</p>")
                                 .arg (version)
                                 .arg (DOWNLOAD_URL));
}

void
MainWindow::appendLogInfo (const QString& text) {
    appendLogRaw (getTimeStamp () + " INFO: " + text);
}

void
MainWindow::appendLogDebug (const QString& text) {
    if (appConfig ().logLevel () >= 4) {
        appendLogRaw (getTimeStamp () + " DEBUG: " + text);
    }
}

void
MainWindow::appendLogError (const QString& text) {
    appendLogRaw (getTimeStamp () + " ERROR: " + text);
}

void
MainWindow::appendLogRaw (const QString& text) {
    foreach (QString line, text.split (QRegExp ("\r|\n|\r\n"))) {
        if (!line.isEmpty ()) {
            m_pLogOutput->append (line);
            updateFromLogLine (line);
        }
    }
}

void
MainWindow::updateFromLogLine (const QString& line) {
    // TODO: this code makes Andrew cry
    checkConnected (line);
    checkFingerprint (line);
    checkLicense (line);
}

void
MainWindow::checkConnected (const QString& line) {
    // TODO: implement ipc connection state messages to replace this hack.
    if (line.contains ("started server") ||
        line.contains ("connected to server") ||
        line.contains ("watchdog status: ok")) {
        setSynergyState (synergyConnected);

        if (!appConfig ().startedBefore () && isVisible ()) {
            QMessageBox::information (
                this,
                "Synergy",
                tr ("Synergy is now connected. You can close the "
                    "config window and Synergy will remain connected in "
                    "the background."));

            appConfig ().setStartedBefore (true);
            appConfig ().saveSettings ();
        }
    }
}

void
MainWindow::checkLicense (const QString& line) {
    if (line.contains ("trial has expired")) {
        licenseManager ().refresh ();
        raiseActivationDialog ();
    }
}

void
MainWindow::checkFingerprint (const QString& line) {
    QRegExp fingerprintRegex (".*server fingerprint: ([A-F0-9:]+)");
    if (!fingerprintRegex.exactMatch (line)) {
        return;
    }

    QString fingerprint = fingerprintRegex.cap (1);
    if (Fingerprint::trustedServers ().isTrusted (fingerprint)) {
        return;
    }

    static bool messageBoxAlreadyShown = false;

    if (!messageBoxAlreadyShown) {
        stopSynergy ();

        messageBoxAlreadyShown = true;
        QMessageBox::StandardButton fingerprintReply =
            QMessageBox::information (
                this,
                tr ("Security question"),
                tr ("Do you trust this fingerprint?\n\n"
                    "%1\n\n"
                    "This is a server fingerprint. You should compare this "
                    "fingerprint to the one on your server's screen. If the "
                    "two don't match exactly, then it's probably not the "
                    "server "
                    "you're expecting (it could be a malicious user).\n\n"
                    "To automatically trust this fingerprint for future "
                    "connections, click Yes. To reject this fingerprint and "
                    "disconnect from the server, click No.")
                    .arg (fingerprint),
                QMessageBox::Yes | QMessageBox::No);

        if (fingerprintReply == QMessageBox::Yes) {
            // restart core process after trusting fingerprint.
            Fingerprint::trustedServers ().trust (fingerprint);
            startSynergy ();
        }

        messageBoxAlreadyShown = false;
    }
}

bool
MainWindow::autoHide () {
    if ((appConfig ().processMode () == Desktop) &&
        appConfig ().getAutoHide ()) {
        hide ();
        return true;
    }

    return false;
}

QString
MainWindow::getTimeStamp () {
    QDateTime current = QDateTime::currentDateTime ();
    return '[' + current.toString (Qt::ISODate) + ']';
}

void
MainWindow::restartSynergy () {
    stopSynergy ();
    startSynergy ();
}

void
MainWindow::proofreadInfo () {
    setEdition (m_AppConfig->edition ()); // Why is this here?

    int oldState   = m_SynergyState;
    m_SynergyState = synergyDisconnected;
    setSynergyState ((qSynergyState) oldState);
}

void
MainWindow::showEvent (QShowEvent* event) {
    QMainWindow::showEvent (event);
    emit windowShown ();
}

void
MainWindow::clearLog () {
    m_pLogOutput->clear ();
}

void
MainWindow::startSynergy () {
    SerialKey serialKey = m_LicenseManager->serialKey ();
    time_t currentTime  = ::time (0);
    if (serialKey.isExpired (currentTime)) {
        if (QDialog::Rejected == raiseActivationDialog ()) {
            return;
        }
    }

    bool desktopMode = appConfig ().processMode () == Desktop;
    bool serviceMode = appConfig ().processMode () == Service;

    appendLogDebug ("starting process");
    m_ExpectedRunningState = kStarted;
    setSynergyState (synergyConnecting);

    QString app;
    QStringList args;

    args << "-f"
         << "--no-tray"
         << "--debug" << appConfig ().logLevelText ();


    args << "--name" << getScreenName ();

    if (desktopMode) {
        setSynergyProcess (new QProcess (this));
    } else {
        // tell client/server to talk to daemon through ipc.
        args << "--ipc";

#if defined(Q_OS_WIN)
        // tell the client/server to shut down when a ms windows desk
        // is switched; this is because we may need to elevate or not
        // based on which desk the user is in (login always needs
        // elevation, where as default desk does not).
        // Note that this is only enabled when synergy is set to elevate
        // 'as needed' (e.g. on a UAC dialog popup) in order to prevent
        // unnecessary restarts when synergy was started elevated or
        // when it is not allowed to elevate. In these cases restarting
        // the server is fruitless.
        if (appConfig ().elevateMode () == ElevateAsNeeded) {
            args << "--stop-on-desk-switch";
        }
#endif
    }

#ifndef Q_OS_LINUX

    if (m_ServerConfig.enableDragAndDrop ()) {
        args << "--enable-drag-drop";
    }

#endif

    if (m_AppConfig->getCryptoEnabled ()) {
        args << "--enable-crypto";
    }

#if defined(Q_OS_WIN)
    // on windows, the profile directory changes depending on the user that
    // launched the process (e.g. when launched with elevation). setting the
    // profile dir on launch ensures it uses the same profile dir is used
    // no matter how its relaunched.
    args << "--profile-dir" << getProfileRootForArg ();
#endif

    if ((synergyType () == synergyClient && !clientArgs (args, app)) ||
        (synergyType () == synergyServer && !serverArgs (args, app))) {
        stopSynergy ();
        return;
    }

    if (desktopMode) {
        connect (synergyProcess (),
                 SIGNAL (finished (int, QProcess::ExitStatus)),
                 this,
                 SLOT (synergyFinished (int, QProcess::ExitStatus)));
        connect (synergyProcess (),
                 SIGNAL (readyReadStandardOutput ()),
                 this,
                 SLOT (logOutput ()));
        connect (synergyProcess (),
                 SIGNAL (readyReadStandardError ()),
                 this,
                 SLOT (logError ()));
    }

    // put a space between last log output and new instance.
    if (!m_pLogOutput->toPlainText ().isEmpty ())
        appendLogRaw ("");

    appendLogInfo ("starting " + QString (synergyType () == synergyServer
                                              ? "server"
                                              : "client"));

    qDebug () << args;

    // show command if debug log level...
    if (appConfig ().logLevel () >= 4) {
        appendLogInfo (QString ("command: %1 %2").arg (app, args.join (" ")));
    }

    appendLogInfo ("config file: " + configFilename ());
    appendLogInfo ("log level: " + appConfig ().logLevelText ());

    if (appConfig ().logToFile ())
        appendLogInfo ("log file: " + appConfig ().logFilename ());

    if (desktopMode) {
        synergyProcess ()->start (app, args);
        if (!synergyProcess ()->waitForStarted ()) {
            show ();
            QMessageBox::warning (
                this,
                tr ("Program can not be started"),
                QString (tr ("The executable<br><br>%1<br><br>could not be "
                             "successfully started, although it does exist. "
                             "Please check if you have sufficient permissions "
                             "to run this program.")
                             .arg (app)));
            return;
        }
    }

    if (serviceMode) {
        QString command (app + " " + args.join (" "));
        m_IpcClient.sendCommand (command, appConfig ().elevateMode ());
    }
}

void
MainWindow::sslToggled (bool enabled) {
    if (enabled) {
        m_pSslCertificate = new SslCertificate (this);
        m_pSslCertificate->generateCertificate ();
    }
    updateLocalFingerprint ();
}

bool
MainWindow::clientArgs (QStringList& args, QString& app) {
    app = appPath (appConfig ().synergycName ());

    if (!QFile::exists (app)) {
        show ();
        QMessageBox::warning (
            this,
            tr ("Synergy client not found"),
            tr ("The executable for the synergy client does not exist."));
        return false;
    }

#if defined(Q_OS_WIN)
    // wrap in quotes so a malicious user can't start \Program.exe as admin.
    app = QString ("\"%1\"").arg (app);
#endif

    if (appConfig ().logToFile ()) {
        appConfig ().persistLogDir ();
        args << "--log" << appConfig ().logFilenameCmd ();
    }

    // check auto config first, if it is disabled or no server detected,
    // use line edit host name if it is not empty
    if (m_pCheckBoxAutoConfig->isChecked ()) {
        if (m_pComboServerList->count () != 0) {
            QString serverIp = m_pComboServerList->currentText ();
            args << serverIp + ":" + QString::number (appConfig ().port ());
            return true;
        }
    }

    if (m_pLineEditHostname->text ().isEmpty ()) {
        show ();
        if (!m_SuppressEmptyServerWarning) {
            QMessageBox::warning (this,
                                  tr ("Hostname is empty"),
                                  tr ("Please fill in a hostname for the "
                                      "synergy client to connect to."));
        }
        return false;
    }

    args << m_pLineEditHostname->text () + ":" +
                QString::number (appConfig ().port ());

    return true;
}

QString
MainWindow::configFilename () {
    QString filename;
    if (m_pRadioInternalConfig->isChecked ()) {
        // TODO: no need to use a temporary file, since we need it to
        // be permenant (since it'll be used for Windows services, etc).
        m_pTempConfigFile = new QTemporaryFile ();
        if (!m_pTempConfigFile->open ()) {
            QMessageBox::critical (
                this,
                tr ("Cannot write configuration file"),
                tr ("The temporary configuration file required to start "
                    "synergy can not be written."));
            return "";
        }

        serverConfig ().save (*m_pTempConfigFile);
        filename = m_pTempConfigFile->fileName ();

        m_pTempConfigFile->close ();
    } else {
        if (!QFile::exists (m_pLineEditConfigFile->text ())) {
            if (QMessageBox::warning (
                    this,
                    tr ("Configuration filename invalid"),
                    tr ("You have not filled in a valid configuration file for "
                        "the synergy server. "
                        "Do you want to browse for the configuration file "
                        "now?"),
                    QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes ||
                !on_m_pButtonBrowseConfigFile_clicked ())
                return "";
        }

        filename = m_pLineEditConfigFile->text ();
    }
    return filename;
}

QString
MainWindow::address () {
    QString i = appConfig ().networkInterface ();
    return (!i.isEmpty () ? i : "") + ":" +
           QString::number (appConfig ().port ());
}

QString
MainWindow::appPath (const QString& name) {
    return appConfig ().synergyProgramDir () + name;
}

bool
MainWindow::serverArgs (QStringList& args, QString& app) {
    app = appPath (appConfig ().synergysName ());

    if (!QFile::exists (app)) {
        QMessageBox::warning (
            this,
            tr ("Synergy server not found"),
            tr ("The executable for the synergy server does not exist."));
        return false;
    }

#if defined(Q_OS_WIN)
    // wrap in quotes so a malicious user can't start \Program.exe as admin.
    app = QString ("\"%1\"").arg (app);
#endif

    if (appConfig ().logToFile ()) {
        appConfig ().persistLogDir ();

        args << "--log" << appConfig ().logFilenameCmd ();
    }

    QString configFilename = this->configFilename ();
#if defined(Q_OS_WIN)
    // wrap in quotes in case username contains spaces.
    configFilename = QString ("\"%1\"").arg (configFilename);
#endif
    args << "-c" << configFilename << "--address" << address ();

    if (!appConfig ().serialKey ().isEmpty ()) {
        args << "--serial-key" << appConfig ().serialKey ();
    }

    return true;
}

void
MainWindow::stopSynergy () {
    appendLogDebug ("stopping process");

    m_ExpectedRunningState = kStopped;

    if (appConfig ().processMode () == Service) {
        stopService ();
    } else if (appConfig ().processMode () == Desktop) {
        stopDesktop ();
    }

    setSynergyState (synergyDisconnected);

// HACK: deleting the object deletes the physical file, which is
// bad, since it could be in use by the Windows service!
#if !defined(Q_OS_WIN)
    delete m_pTempConfigFile;
#endif
    m_pTempConfigFile = NULL;

    // reset so that new connects cause auto-hide.
    m_AlreadyHidden = false;
}

void
MainWindow::stopService () {
    // send empty command to stop service from laucning anything.
    m_IpcClient.sendCommand ("", appConfig ().elevateMode ());
}

void
MainWindow::stopDesktop () {
    QMutexLocker locker (&m_StopDesktopMutex);
    if (!synergyProcess ()) {
        return;
    }

    appendLogInfo ("stopping synergy desktop process");

    if (synergyProcess ()->isOpen ()) {
        synergyProcess ()->close ();
    }

    delete synergyProcess ();
    setSynergyProcess (NULL);
}

void
MainWindow::synergyFinished (int exitCode, QProcess::ExitStatus) {
    if (exitCode == 0) {
        appendLogInfo (QString ("process exited normally"));
    } else {
        appendLogError (
            QString ("process exited with error code: %1").arg (exitCode));
    }

    if (m_ExpectedRunningState == kStarted) {
        QTimer::singleShot (1000, this, SLOT (startSynergy ()));
        appendLogInfo (
            QString ("detected process not running, auto restarting"));
    } else {
        setSynergyState (synergyDisconnected);
    }
}

void
MainWindow::setSynergyState (qSynergyState state) {
    if (synergyState () == state)
        return;

    if (state == synergyConnected || state == synergyConnecting) {
        disconnect (m_pButtonToggleStart,
                    SIGNAL (clicked ()),
                    m_pActionStartSynergy,
                    SLOT (trigger ()));
        connect (m_pButtonToggleStart,
                 SIGNAL (clicked ()),
                 m_pActionStopSynergy,
                 SLOT (trigger ()));
        m_pButtonToggleStart->setText (tr ("&Stop"));
        m_pButtonApply->setEnabled (true);
    } else if (state == synergyDisconnected) {
        disconnect (m_pButtonToggleStart,
                    SIGNAL (clicked ()),
                    m_pActionStopSynergy,
                    SLOT (trigger ()));
        connect (m_pButtonToggleStart,
                 SIGNAL (clicked ()),
                 m_pActionStartSynergy,
                 SLOT (trigger ()));
        m_pButtonToggleStart->setText (tr ("&Start"));
        m_pButtonApply->setEnabled (false);
    }

    bool connected = false;
    if (state == synergyConnected || state == synergyTransfering) {
        connected = true;
    }

    m_pActionStartSynergy->setEnabled (!connected);
    m_pActionStopSynergy->setEnabled (connected);

    switch (state) {
        case synergyConnected: {
            if (m_AppConfig->getCryptoEnabled ()) {
                m_pLabelPadlock->show ();
            } else {
                m_pLabelPadlock->hide ();
            }

            setStatus (tr ("Synergy is running."));

            break;
        }
        case synergyConnecting:
            m_pLabelPadlock->hide ();
            setStatus (tr ("Synergy is starting."));
            break;
        case synergyDisconnected:
            m_pLabelPadlock->hide ();
            setStatus (tr ("Synergy is not running."));
            break;
        case synergyTransfering:
            break;
    }

    setIcon (state);

    m_SynergyState = state;
}

void
MainWindow::setVisible (bool visible) {
    QMainWindow::setVisible (visible);
    m_pActionMinimize->setEnabled (visible);
    m_pActionRestore->setEnabled (!visible);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 // lion
    // dock hide only supported on lion :(
    ProcessSerialNumber psn = {0, kCurrentProcess};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    GetCurrentProcess (&psn);
#pragma GCC diagnostic pop
    if (visible)
        TransformProcessType (&psn, kProcessTransformToForegroundApplication);
    else
        TransformProcessType (&psn, kProcessTransformToBackgroundApplication);
#endif
}

QString
MainWindow::getIPAddresses () {
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses ();

    bool hinted = false;
    QString result;
    for (int i = 0; i < addresses.size (); i++) {
        if (addresses[i].protocol () == QAbstractSocket::IPv4Protocol &&
            addresses[i] != QHostAddress (QHostAddress::LocalHost)) {

            QString address = addresses[i].toString ();
            QString format  = "%1, ";

            // usually 192.168.x.x is a useful ip for the user, so indicate
            // this by making it bold.
            if (!hinted && address.startsWith ("192.168")) {
                hinted = true;
                format = "<b>%1</b>, ";
            }

            result += format.arg (address);
        }
    }

    if (result == "") {
        return tr ("Unknown");
    }

    // remove trailing comma.
    result.chop (2);

    return result;
}

QString
MainWindow::getScreenName () {
    if (appConfig ().screenName () == "") {
        return QHostInfo::localHostName ();
    } else {
        return appConfig ().screenName ();
    }
}

void
MainWindow::changeEvent (QEvent* event) {
    if (event != 0) {
        switch (event->type ()) {
            case QEvent::LanguageChange: {
                retranslateUi (this);
                retranslateMenuBar ();

                proofreadInfo ();

                break;
            }
            default:
                QMainWindow::changeEvent (event);
        }
    }
}

void
MainWindow::updateZeroconfService () {
    QMutexLocker locker (&m_UpdateZeroconfMutex);

    if (isBonjourRunning ()) {
        if (!m_AppConfig->wizardShouldRun ()) {
            if (m_pZeroconfService) {
                delete m_pZeroconfService;
                m_pZeroconfService = NULL;
            }

            if (m_AppConfig->autoConfig () || synergyType () == synergyServer) {
                m_pZeroconfService = new ZeroconfService (this);
            }
        }
    }
}

void
MainWindow::serverDetected (const QString name) {
    if (m_pComboServerList->findText (name) == -1) {
        // Note: the first added item triggers startSynergy
        m_pComboServerList->addItem (name);
    }

    if (m_pComboServerList->count () > 1) {
        m_pComboServerList->show ();
    }
}

void
MainWindow::setEdition (Edition edition) {
    setWindowTitle (m_LicenseManager->getEditionName (edition));
    if (m_AppConfig->getCryptoEnabled ()) {
        m_pSslCertificate = new SslCertificate (this);
        m_pSslCertificate->generateCertificate ();
    }
    updateLocalFingerprint ();
    saveSettings ();
}

void
MainWindow::beginTrial (bool isExpiring) {
    // Hack
    // if (isExpiring) {
    time_t daysLeft = m_LicenseManager->serialKey ().daysLeft (::time (0));
    QString expiringNotice (
        "<html><head/><body><p><span style=\""
        "font-weight:600;\">%1</span> day%3 of "
        "your %2 trial remain%5. <a href="
        "\"https://symless.com/synergy/trial/thanks?id=%4\">"
        "<span style=\"text-decoration: underline;"
        " color:#0000ff;\">Buy now!</span></a>"
        "</p></body></html>");
    expiringNotice = expiringNotice.arg (daysLeft)
                         .arg (LicenseManager::getEditionName (
                             m_LicenseManager->activeEdition ()))
                         .arg ((daysLeft == 1) ? "" : "s")
                         .arg (QString::fromStdString (
                             m_LicenseManager->serialKey ().toString ()))
                         .arg ((daysLeft == 1) ? "s" : "");
    this->m_trialLabel->setText (expiringNotice);
    this->m_trialWidget->show ();
    //}
    setWindowTitle (m_LicenseManager->activeEditionName ());
}

void
MainWindow::endTrial (bool isExpired) {
    if (isExpired) {
        QString expiredNotice (
            "<html><head/><body><p>Your %1 trial has expired. <a href="
            "\"https://symless.com/synergy/trial/thanks?id=%2\">"
            "<span style=\"text-decoration: underline;color:#0000ff;\">"
            "Buy now!</span></a></p></body></html>");
        expiredNotice = expiredNotice
                            .arg (LicenseManager::getEditionName (
                                m_LicenseManager->activeEdition ()))
                            .arg (QString::fromStdString (
                                m_LicenseManager->serialKey ().toString ()));

        this->m_trialLabel->setText (expiredNotice);
        this->m_trialWidget->show ();
        stopSynergy ();
        m_AppConfig->activationHasRun (false);
    } else {
        this->m_trialWidget->hide ();
    }
    setWindowTitle (m_LicenseManager->activeEditionName ());
}

void
MainWindow::updateLocalFingerprint () {
    if (m_AppConfig->getCryptoEnabled () &&
        Fingerprint::local ().fileExists ()) {
        m_pLabelFingerprint->setVisible (true);
        m_pLabelLocalFingerprint->setVisible (true);
        m_pLabelLocalFingerprint->setText (Fingerprint::local ().readFirst ());
    } else {
        m_pLabelFingerprint->setVisible (false);
        m_pLabelLocalFingerprint->setVisible (false);
    }
}

LicenseManager&
MainWindow::licenseManager () const {
    return *m_LicenseManager;
}

void
MainWindow::on_m_pGroupClient_toggled (bool on) {
    m_pGroupServer->setChecked (!on);
    if (on) {
        updateZeroconfService ();
    }
}

void
MainWindow::on_m_pGroupServer_toggled (bool on) {
    m_pGroupClient->setChecked (!on);
    if (on) {
        updateZeroconfService ();
    }
}

bool
MainWindow::on_m_pButtonBrowseConfigFile_clicked () {
    QString fileName =
        QFileDialog::getOpenFileName (this,
                                      tr ("Browse for a synergys config file"),
                                      QString (),
                                      synergyConfigFilter);

    if (!fileName.isEmpty ()) {
        m_pLineEditConfigFile->setText (fileName);
        return true;
    }

    return false;
}

bool
MainWindow::on_m_pActionSave_triggered () {
    QString fileName =
        QFileDialog::getSaveFileName (this, tr ("Save configuration as..."));

    if (!fileName.isEmpty () && !serverConfig ().save (fileName)) {
        QMessageBox::warning (this,
                              tr ("Save failed"),
                              tr ("Could not save configuration to file."));
        return true;
    }

    return false;
}

void
MainWindow::on_m_pActionAbout_triggered () {
    AboutDialog dlg (this, appPath (appConfig ().synergycName ()));
    dlg.exec ();
}

void
MainWindow::on_m_pActionSettings_triggered () {
    ProcessMode lastProcessMode = appConfig ().processMode ();

    SettingsDialog dlg (this, appConfig ());
    dlg.exec ();

    if (lastProcessMode != appConfig ().processMode ()) {
        onModeChanged (true, true);
    }
}

void
MainWindow::autoAddScreen (const QString name) {
    if (!m_ServerConfig.ignoreAutoConfigClient ()) {
        if (m_ActivationDialogRunning) {
            // TODO: refactor this code
            // add this screen to the pending list and check this list until
            // users finish activation dialog
            m_PendingClientNames.append (name);
            return;
        }

        int r = m_ServerConfig.autoAddScreen (name);
        if (r != kAutoAddScreenOk) {
            switch (r) {
                case kAutoAddScreenManualServer:
                    showConfigureServer (
                        tr ("Please add the server (%1) to the grid.")
                            .arg (appConfig ().screenName ()));
                    break;

                case kAutoAddScreenManualClient:
                    showConfigureServer (
                        tr ("Please drag the new client screen (%1) "
                            "to the desired position on the grid.")
                            .arg (name));
                    break;
            }
        } else {
            restartSynergy ();
        }
    }
}

void
MainWindow::showConfigureServer (const QString& message) {
    ServerConfigDialog dlg (this, serverConfig (), appConfig ().screenName ());
    dlg.message (message);
    dlg.exec ();
}

void
MainWindow::on_m_pButtonConfigureServer_clicked () {
    showConfigureServer ();
}

void
MainWindow::on_m_pActivate_triggered () {
    raiseActivationDialog ();
}

void
MainWindow::on_m_pButtonApply_clicked () {
    restartSynergy ();
}

#if defined(Q_OS_WIN)
bool
MainWindow::isServiceRunning (QString name) {
    SC_HANDLE hSCManager;
    hSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager == NULL) {
        appendLogError ("failed to open a service controller manager, error: " +
                        GetLastError ());
        return false;
    }

    auto array = name.toLocal8Bit ();
    SC_HANDLE hService =
        OpenService (hSCManager, array.data (), SERVICE_QUERY_STATUS);

    if (hService == NULL) {
        appendLogDebug ("failed to open service: " + name);
        return false;
    }

    SERVICE_STATUS status;
    if (QueryServiceStatus (hService, &status)) {
        if (status.dwCurrentState == SERVICE_RUNNING) {
            return true;
        }
    }
#else
bool
MainWindow::isServiceRunning () {
#endif
    return false;
}

bool
MainWindow::isBonjourRunning () {
    bool result = false;

#if defined(Q_OS_WIN)
    result = isServiceRunning ("Bonjour Service");
#else
    result = true;
#endif

    return result;
}

void
MainWindow::downloadBonjour () {
#if defined(Q_OS_WIN)
    QUrl url;
    int arch = getProcessorArch ();
    if (arch == kProcessorArchWin32) {
        url.setUrl (bonjourBaseUrl + bonjourFilename32);
        appendLogInfo ("downloading 32-bit Bonjour");
    } else if (arch == kProcessorArchWin64) {
        url.setUrl (bonjourBaseUrl + bonjourFilename64);
        appendLogInfo ("downloading 64-bit Bonjour");
    } else {
        QMessageBox::critical (
            this, tr ("Synergy"), tr ("Failed to detect system architecture."));
        return;
    }

    if (m_pDataDownloader == NULL) {
        m_pDataDownloader = new DataDownloader (this);
        connect (m_pDataDownloader,
                 SIGNAL (isComplete ()),
                 SLOT (installBonjour ()));
    }

    m_pDataDownloader->download (url);

    if (m_DownloadMessageBox == NULL) {
        m_DownloadMessageBox = new QMessageBox (this);
        m_DownloadMessageBox->setWindowTitle ("Synergy");
        m_DownloadMessageBox->setIcon (QMessageBox::Information);
        m_DownloadMessageBox->setText ("Installing Bonjour, please wait...");
        m_DownloadMessageBox->setStandardButtons (0);
        m_pCancelButton = m_DownloadMessageBox->addButton (
            tr ("Cancel"), QMessageBox::RejectRole);
    }

    m_DownloadMessageBox->exec ();

    if (m_DownloadMessageBox->clickedButton () == m_pCancelButton) {
        m_pDataDownloader->cancel ();
    }
#endif
}

void
MainWindow::installBonjour () {
#if defined(Q_OS_WIN)
#if QT_VERSION >= 0x050000
    QString tempLocation =
        QStandardPaths::writableLocation (QStandardPaths::TempLocation);
#else
    QString tempLocation =
        QDesktopServices::storageLocation (QDesktopServices::TempLocation);
#endif
    QString filename = tempLocation;
    filename.append ("\\").append (bonjourTargetFilename);
    QFile file (filename);
    if (!file.open (QIODevice::WriteOnly)) {
        m_DownloadMessageBox->hide ();

        QMessageBox::warning (
            this,
            "Synergy",
            tr ("Failed to download Bonjour installer to location: %1")
                .arg (tempLocation));
        return;
    }

    file.write (m_pDataDownloader->data ());
    file.close ();

    QStringList arguments;
    arguments.append ("/i");
    QString winFilename = QDir::toNativeSeparators (filename);
    arguments.append (winFilename);
    arguments.append ("/passive");
    if (m_BonjourInstall == NULL) {
        m_BonjourInstall = new CommandProcess ("msiexec", arguments);
    }

    QThread* thread = new QThread;
    connect (m_BonjourInstall,
             SIGNAL (finished ()),
             this,
             SLOT (bonjourInstallFinished ()));
    connect (m_BonjourInstall, SIGNAL (finished ()), thread, SLOT (quit ()));
    connect (thread, SIGNAL (finished ()), thread, SLOT (deleteLater ()));

    m_BonjourInstall->moveToThread (thread);
    thread->start ();

    QMetaObject::invokeMethod (m_BonjourInstall, "run", Qt::QueuedConnection);

    m_DownloadMessageBox->hide ();
#endif
}

void
MainWindow::promptAutoConfig () {
    if (!isBonjourRunning ()) {
        int r = QMessageBox::question (
            this,
            tr ("Synergy"),
            tr ("Do you want to enable auto config and install Bonjour?\n\n"
                "This feature helps you establish the connection."),
            QMessageBox::Yes | QMessageBox::No);

        if (r == QMessageBox::Yes) {
            m_AppConfig->setAutoConfig (true);
            downloadBonjour ();
        } else {
            m_AppConfig->setAutoConfig (false);
            m_pCheckBoxAutoConfig->setChecked (false);
        }
    }

    m_AppConfig->setAutoConfigPrompted (true);
}

void
MainWindow::on_m_pComboServerList_currentIndexChanged (QString) {
    if (m_pComboServerList->count () != 0) {
        restartSynergy ();
    }
}

void
MainWindow::on_m_pCheckBoxAutoConfig_toggled (bool checked) {
    if (!isBonjourRunning () && checked) {
        if (!m_SuppressAutoConfigWarning) {
            int r = QMessageBox::information (
                this,
                tr ("Synergy"),
                tr ("Auto config feature requires Bonjour.\n\n"
                    "Do you want to install Bonjour?"),
                QMessageBox::Yes | QMessageBox::No);

            if (r == QMessageBox::Yes) {
                downloadBonjour ();
            }
        }

        m_pCheckBoxAutoConfig->setChecked (false);
        return;
    }

    m_pLineEditHostname->setDisabled (checked);
    appConfig ().setAutoConfig (checked);
    updateZeroconfService ();

    if (!checked) {
        m_pComboServerList->clear ();
        m_pComboServerList->hide ();
    }
}

void
MainWindow::bonjourInstallFinished () {
    appendLogInfo ("Bonjour install finished");

    m_pCheckBoxAutoConfig->setChecked (true);
}

int
MainWindow::raiseActivationDialog () {
    if (m_ActivationDialogRunning) {
        return QDialog::Rejected;
    }
    ActivationDialog activationDialog (this, appConfig (), licenseManager ());
    m_ActivationDialogRunning = true;
    connect (&activationDialog,
             SIGNAL (finished (int)),
             this,
             SLOT (on_activationDialogFinish ()),
             Qt::QueuedConnection);
    int result                = activationDialog.exec ();
    m_ActivationDialogRunning = false;
    if (!m_PendingClientNames.empty ()) {
        foreach (const QString& name, m_PendingClientNames) {
            autoAddScreen (name);
        }

        m_PendingClientNames.clear ();
    }
    if (result == QDialog::Accepted) {
        restartSynergy ();
    }
    return result;
}

void
MainWindow::on_windowShown () {
    time_t currentTime = ::time (0);
    if (!m_AppConfig->activationHasRun () &&
        ((m_AppConfig->edition () == kUnregistered) ||
         (m_LicenseManager->serialKey ().isExpired (currentTime)))) {
        raiseActivationDialog ();
    }
}

QString
MainWindow::getProfileRootForArg () {
    CoreInterface coreInterface;
    QString dir = coreInterface.getProfileDir ();

// HACK: strip our app name since we're returning the root dir.
#if defined(Q_OS_WIN)
    dir.replace ("\\Synergy", "");
#else
    dir.replace ("/.synergy", "");
#endif

    return QString ("\"%1\"").arg (dir);
}

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
#define HELP_URL     "http://symless.com/help?source=gui"

#include <iostream>

#include "MainWindow.h"

#include "Fingerprint.h"
#include "AboutDialog.h"
#include "ServerConfigDialog.h"
#include "SettingsDialog.h"
#include "ActivationDialog.h"
#include "DataDownloader.h"
#include "CommandProcess.h"
#include "LicenseManager.h"
#include <shared/EditionType.h>
#include "QUtility.h"
#include "ProcessorArch.h"
#include "SslCertificate.h"
#include "Zeroconf.h"
#include <QPushButton>

#if defined(Q_OS_MAC)
#include "OSXHelpers.h"
#endif

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
static const char synergyConfigName[] = "synergy.sgc";
static const QString synergyConfigFilter(QObject::tr("Synergy Configurations (*.sgc);;All files (*.*)"));
#else
static const char synergyConfigName[] = "synergy.conf";
static const QString synergyConfigFilter(QObject::tr("Synergy Configurations (*.conf);;All files (*.*)"));
#endif

static const char* tlsCheckString = "network encryption protocol: ";

static const int debugLogLevel = 1;

static const char* synergyLightIconFiles[] =
{
    ":/res/icons/64x64/synergy-light-disconnected.png",
    ":/res/icons/64x64/synergy-light-disconnected.png",
    ":/res/icons/64x64/synergy-light-connected.png",
    ":/res/icons/64x64/synergy-light-transfering.png",
    ":/res/icons/64x64/synergy-light-disconnected.png"
};

static const char* synergyDarkIconFiles[] =
{
    ":/res/icons/64x64/synergy-dark-disconnected.png",
    ":/res/icons/64x64/synergy-dark-disconnected.png",
    ":/res/icons/64x64/synergy-dark-connected.png",
    ":/res/icons/64x64/synergy-dark-transfering.png",
    ":/res/icons/64x64/synergy-dark-disconnected.png"    //synergyPendingRetry
};

static const char* synergyDefaultIconFiles[] =
{
    ":/res/icons/16x16/synergy-disconnected.png",   //synergyDisconnected
    ":/res/icons/16x16/synergy-disconnected.png",   //synergyConnecting
    ":/res/icons/16x16/synergy-connected.png",      //synergyConnected
    ":/res/icons/16x16/synergy-transfering.png",    //synergyListening
    ":/res/icons/16x16/synergy-disconnected.png"    //synergyPendingRetry
};

#ifdef SYNERGY_ENTERPRISE
MainWindow::MainWindow (AppConfig& appConfig)
#else
MainWindow::MainWindow (AppConfig& appConfig,
                        LicenseManager& licenseManager)
#endif
:
#ifndef SYNERGY_ENTERPRISE
    m_LicenseManager(&licenseManager),
    m_ActivationDialogRunning(false),
#endif
    m_pZeroconf(nullptr),
    m_AppConfig(&appConfig),
    m_pSynergy(NULL),
    m_SynergyState(synergyDisconnected),
    m_ServerConfig(5, 3, m_AppConfig->screenName(), this),
    m_pTrayIcon(NULL),
    m_pTrayIconMenu(NULL),
    m_AlreadyHidden(false),
    m_pMenuBar(NULL),
    m_pMenuFile(NULL),
    m_pMenuEdit(NULL),
    m_pMenuWindow(NULL),
    m_pMenuHelp(NULL),
    m_pCancelButton(NULL),
    m_ExpectedRunningState(kStopped),
    m_pSslCertificate(NULL),
    m_SecureSocket(false)
{
#ifndef SYNERGY_ENTERPRISE
    m_pZeroconf = new Zeroconf(this);
#endif

    setupUi(this);

    createMenuBar();
    loadSettings();
    initConnections();

    m_pWidgetUpdate->hide();
    m_VersionChecker.setApp(appPath(appConfig.synergycName()));
    m_pLabelScreenName->setText(getScreenName());
    m_pLabelIpAddresses->setText(getIPAddresses());

#if defined(Q_OS_WIN)
    // ipc must always be enabled, so that we can disable command when switching to desktop mode.
    connect(&m_IpcClient, SIGNAL(readLogLine(const QString&)), this, SLOT(appendLogRaw(const QString&)));
    connect(&m_IpcClient, SIGNAL(errorMessage(const QString&)), this, SLOT(appendLogError(const QString&)));
    connect(&m_IpcClient, SIGNAL(infoMessage(const QString&)), this, SLOT(appendLogInfo(const QString&)));
    m_IpcClient.connectToHost();
#endif

    // change default size based on os
#if defined(Q_OS_MAC)
    resize(720, 550);
    setMinimumSize(size());
#elif defined(Q_OS_LINUX)
    resize(700, 530);
    setMinimumSize(size());
#endif

    m_trialWidget->hide();

    // hide padlock icon
    secureSocket(false);

    updateLocalFingerprint();

    connect (this, SIGNAL(windowShown()),
             this, SLOT(on_windowShown()), Qt::QueuedConnection);
#ifndef SYNERGY_ENTERPRISE
    connect (m_LicenseManager, SIGNAL(editionChanged(Edition)),
             this, SLOT(setEdition(Edition)), Qt::QueuedConnection);

    connect (m_LicenseManager, SIGNAL(showLicenseNotice(QString)),
             this, SLOT(showLicenseNotice(QString)), Qt::QueuedConnection);

    connect (m_LicenseManager, SIGNAL(InvalidLicense()),
             this, SLOT(InvalidLicense()), Qt::QueuedConnection);
#endif

    connect (m_AppConfig, SIGNAL(sslToggled()),
             this, SLOT(updateLocalFingerprint()), Qt::QueuedConnection);

    connect (m_AppConfig, SIGNAL(zeroConfToggled()),
             this, SLOT(zeroConfToggled()), Qt::QueuedConnection);

#ifdef SYNERGY_ENTERPRISE
    setWindowTitle ("Synergy 1 Enterprise");
#else
    setWindowTitle (m_LicenseManager->activeEditionName());
    m_LicenseManager->refresh();
#endif

    QString lastVersion = m_AppConfig->lastVersion();
    QString currentVersion = m_VersionChecker.getVersion();
    if (lastVersion != currentVersion) {
        m_AppConfig->setLastVersion (currentVersion);
#ifndef SYNERGY_ENTERPRISE
        m_LicenseManager->notifyUpdate (lastVersion, currentVersion);
#endif
    }

#ifdef SYNERGY_ENTERPRISE
    m_pActivate->setVisible(false);
#endif

#ifndef SYNERGY_ENTERPRISE
    updateZeroconfService();

    addZeroconfServer(m_AppConfig->autoConfigServer());
#endif

    updateAutoConfigWidgets();
}

MainWindow::~MainWindow()
{
    if (appConfig().processMode() == Desktop) {
        m_ExpectedRunningState = kStopped;
        stopDesktop();
    }

#ifndef SYNERGY_ENTERPRISE
    delete m_pZeroconf;
#endif

    delete m_pSslCertificate;
}

void MainWindow::open()
{
    createTrayIcon();

    if (appConfig().getAutoHide()) {
        hide();
    } else {
        showNormal();
    }

    m_VersionChecker.checkLatest();

    // only start if user has previously started. this stops the gui from
    // auto hiding before the user has configured synergy (which of course
    // confuses first time users, who think synergy has crashed).
    if (appConfig().startedBefore() && appConfig().processMode() == Desktop) {
        startSynergy();
    }
}

void MainWindow::setStatus(const QString &status)
{
    m_pStatusLabel->setText(status);
}

void MainWindow::createTrayIcon()
{
    m_pTrayIconMenu = new QMenu(this);

    m_pTrayIconMenu->addAction(m_pActionStartSynergy);
    m_pTrayIconMenu->addAction(m_pActionStopSynergy);
    m_pTrayIconMenu->addSeparator();

    m_pTrayIconMenu->addAction(m_pActionMinimize);
    m_pTrayIconMenu->addAction(m_pActionRestore);
    m_pTrayIconMenu->addSeparator();
    m_pTrayIconMenu->addAction(m_pActionQuit);

    m_pTrayIcon = new QSystemTrayIcon(this);
    m_pTrayIcon->setContextMenu(m_pTrayIconMenu);
    m_pTrayIcon->setToolTip("Synergy");

    connect(m_pTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));

    setIcon(synergyDisconnected);

    m_pTrayIcon->show();
}

void MainWindow::retranslateMenuBar()
{
    m_pMenuFile->setTitle(tr("&File"));
    m_pMenuEdit->setTitle(tr("&Edit"));
    m_pMenuWindow->setTitle(tr("&Window"));
    m_pMenuHelp->setTitle(tr("&Help"));
}

void MainWindow::createMenuBar()
{
    m_pMenuBar = new QMenuBar(this);
    m_pMenuFile = new QMenu("", m_pMenuBar);
    m_pMenuEdit = new QMenu("", m_pMenuBar);
    m_pMenuWindow = new QMenu("", m_pMenuBar);
    m_pMenuHelp = new QMenu("", m_pMenuBar);
    retranslateMenuBar();

    m_pMenuBar->addAction(m_pMenuFile->menuAction());
    m_pMenuBar->addAction(m_pMenuEdit->menuAction());
#if !defined(Q_OS_MAC)
    m_pMenuBar->addAction(m_pMenuWindow->menuAction());
#endif
    m_pMenuBar->addAction(m_pMenuHelp->menuAction());

    m_pMenuFile->addAction(m_pActionStartSynergy);
    m_pMenuFile->addAction(m_pActionStopSynergy);
    m_pMenuFile->addSeparator();
    m_pMenuFile->addAction(m_pActivate);
    m_pMenuFile->addSeparator();
    m_pMenuFile->addAction(m_pActionSave);
    m_pMenuFile->addSeparator();
    m_pMenuFile->addAction(m_pActionQuit);
    m_pMenuEdit->addAction(m_pActionSettings);
    m_pMenuWindow->addAction(m_pActionMinimize);
    m_pMenuWindow->addAction(m_pActionRestore);
    m_pMenuHelp->addAction(m_pActionAbout);
    m_pMenuHelp->addAction(m_pActionHelp);


    setMenuBar(m_pMenuBar);
}

void MainWindow::loadSettings()
{
    // the next two must come BEFORE loading groupServerChecked and groupClientChecked or
    // disabling and/or enabling the right widgets won't automatically work
    m_pRadioExternalConfig->setChecked(appConfig().getUseExternalConfig());
    m_pRadioInternalConfig->setChecked(appConfig().getUseInternalConfig());

    m_pGroupServer->setChecked(appConfig().getServerGroupChecked());
    m_pLineEditConfigFile->setText(appConfig().getConfigFile());
    m_pGroupClient->setChecked(appConfig().getClientGroupChecked());
    m_pLineEditHostname->setText(appConfig().getServerHostname());
}

void MainWindow::initConnections()
{
    connect(m_pActionMinimize, SIGNAL(triggered()), this, SLOT(hide()));
    connect(m_pActionRestore, SIGNAL(triggered()), this, SLOT(showNormal()));
    connect(m_pActionStartSynergy, SIGNAL(triggered()), this, SLOT(startSynergy()));
    connect(m_pActionStopSynergy, SIGNAL(triggered()), this, SLOT(stopSynergy()));
    connect(m_pActionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(&m_VersionChecker, SIGNAL(updateFound(const QString&)), this, SLOT(updateFound(const QString&)));
}

void MainWindow::saveSettings()
{
    // program settings
    appConfig().setServerGroupChecked(m_pGroupServer->isChecked());
    appConfig().setClientGroupChecked(m_pGroupClient->isChecked());
    appConfig().setUseExternalConfig(m_pRadioExternalConfig->isChecked());
    appConfig().setUseInternalConfig(m_pRadioInternalConfig->isChecked());
    appConfig().setConfigFile(m_pLineEditConfigFile->text());
    appConfig().setServerHostname(m_pLineEditHostname->text());


    //Save everything
    GUI::Config::ConfigWriter::make()->globalSave();

}

void MainWindow::zeroConfToggled() {
#ifndef SYNERGY_ENTERPRISE
    updateZeroconfService();

    addZeroconfServer(m_AppConfig->autoConfigServer());

    updateAutoConfigWidgets();
#endif
}
void MainWindow::setIcon(qSynergyState state)
{
    QIcon icon;

#ifdef Q_OS_MAC
    if (isOSXInterfaceStyleDark())
        icon.addFile(synergyDarkIconFiles[state]);
    else
        icon.addFile(synergyLightIconFiles[state]);
#else
    icon.addFile(synergyDefaultIconFiles[state]);
#endif

    if (m_pTrayIcon)
        m_pTrayIcon->setIcon(icon);
}

void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
    {
        if (isVisible())
        {
            hide();
        }
        else
        {
            showNormal();
            activateWindow();
        }
    }
}

void MainWindow::logOutput()
{
    if (m_pSynergy)
    {
        QString text(m_pSynergy->readAllStandardOutput());
        foreach(QString line, text.split(QRegExp("\r|\n|\r\n")))
        {
            if (!line.isEmpty())
            {
                appendLogRaw(line);
            }
        }
    }
}

void MainWindow::logError()
{
    if (m_pSynergy)
    {
        appendLogRaw(m_pSynergy->readAllStandardError());
    }
}

void MainWindow::updateFound(const QString &version)
{
    m_pWidgetUpdate->show();
    m_pLabelUpdate->setText(
        tr("<p>Your version of Synergy is out of date. "
           "Version <b>%1</b> is now available to "
           "<a href=\"%2\">download</a>.</p>")
        .arg(version).arg(DOWNLOAD_URL));
}

void MainWindow::appendLogInfo(const QString& text)
{
    appendLogRaw(getTimeStamp() + " INFO: " + text);
}

void MainWindow::appendLogDebug(const QString& text) {
    if (appConfig().logLevel() >= debugLogLevel) {
        appendLogRaw(getTimeStamp() + " DEBUG: " + text);
    }
}

void MainWindow::appendLogError(const QString& text)
{
    appendLogRaw(getTimeStamp() + " ERROR: " + text);
}

void MainWindow::appendLogRaw(const QString& text)
{
    foreach(QString line, text.split(QRegExp("\r|\n|\r\n"))) {
        if (!line.isEmpty()) {

            // HACK: macOS 10.13.4+ spamming error lines in logs making them
            // impossible to read and debug; giving users a red herring.
            if (line.contains("calling TIS/TSM in non-main thread environment")) {
                continue;
            }

            m_pLogOutput->appendPlainText(line);
            updateFromLogLine(line);
        }
    }
}

void MainWindow::updateFromLogLine(const QString &line)
{
    // TODO: This shouldn't be updating from log needs a better way of doing this
    checkConnected(line);
    checkFingerprint(line);
    checkSecureSocket(line);

#ifndef SYNERGY_ENTERPRISE
    checkLicense(line);
#endif
}

void MainWindow::checkConnected(const QString& line)
{
    // TODO: implement ipc connection state messages to replace this hack.
    if (line.contains("connected to server") ||
        line.contains("accepted client connection"))
    {
        setSynergyState(synergyConnected);

        if (!appConfig().startedBefore() && isVisible()) {
                QMessageBox::information(
                    this, "Synergy",
                    tr("Synergy is now connected. You can close the "
                    "config window and Synergy will remain connected in "
                    "the background."));

            appConfig().setStartedBefore(true);
        }
    }
    else if (line.contains("started server"))
    {
        setSynergyState(synergyListening);
    }
    else if (line.contains("disconnected from server") || line.contains("process exited"))
    {
        setSynergyState(synergyDisconnected);
    }
    else if (line.contains("connecting to"))
    {
        setSynergyState(synergyConnecting);
    }
}

#ifndef SYNERGY_ENTERPRISE
void MainWindow::checkLicense(const QString &line)
{
    if (line.contains("trial has expired")) {
        licenseManager().refresh();
        raiseActivationDialog();
    }
}
#endif

void MainWindow::checkFingerprint(const QString& line)
{
    QRegExp fingerprintRegex(".*server fingerprint: ([A-F0-9:]+)");
    if (!fingerprintRegex.exactMatch(line)) {
        return;
    }

    QString fingerprint = fingerprintRegex.cap(1);
    if (Fingerprint::trustedServers().isTrusted(fingerprint)) {
        return;
    }

    static bool messageBoxAlreadyShown = false;

    if (!messageBoxAlreadyShown) {
        stopSynergy();

        messageBoxAlreadyShown = true;
        QMessageBox::StandardButton fingerprintReply =
            QMessageBox::information(
            this, tr("Security question"),
            tr("You are connecting to a server. Here is it's fingerprint:\n\n"
               "%1\n\n"
               "Compare this fingerprint to the one on your server's screen."
               "If the two don't match exactly, then it's probably not the server "
               "you're expecting (it could be a malicious user).\n\n"
               "To automatically trust this fingerprint for future "
               "connections, click Yes. To reject this fingerprint and "
               "disconnect from the server, click No.")
            .arg(fingerprint),
            QMessageBox::Yes | QMessageBox::No);

        if (fingerprintReply == QMessageBox::Yes) {
            // restart core process after trusting fingerprint.
            Fingerprint::trustedServers().trust(fingerprint);
            startSynergy();
        }

        messageBoxAlreadyShown = false;
    }
}

void MainWindow::checkSecureSocket(const QString& line)
{
    // obviously not very secure, since this can be tricked by injecting something
    // into the log. however, since we don't have IPC between core and GUI... patches welcome.
    const int index = line.indexOf(tlsCheckString, 0, Qt::CaseInsensitive);
    if (index > 0) {
        secureSocket(true);

        //Get the protocol version from the line
        m_SecureSocketVersion = line.mid(index + strlen(tlsCheckString));
    }
}
QString MainWindow::getTimeStamp()
{
    QDateTime current = QDateTime::currentDateTime();
    return '[' + current.toString(Qt::ISODate) + ']';
}

void MainWindow::restartSynergy()
{
    stopSynergy();
    startSynergy();
}

void MainWindow::proofreadInfo()
{
#ifndef SYNERGY_ENTERPRISE
    setEdition(m_AppConfig->edition()); // Why is this here?
#endif
    int oldState = m_SynergyState;
    m_SynergyState = synergyDisconnected;
    setSynergyState((qSynergyState)oldState);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    emit windowShown();
}

void MainWindow::clearLog()
{
    m_pLogOutput->clear();
}

void MainWindow::startSynergy()
{
#ifndef SYNERGY_ENTERPRISE
    SerialKey serialKey = m_LicenseManager->serialKey();
    if (!serialKey.isValid()) {
        if (QDialog::Rejected == raiseActivationDialog()) {
            return;
        }
    }
#endif
    bool desktopMode = appConfig().processMode() == Desktop;
    bool serviceMode = appConfig().processMode() == Service;

    appendLogDebug("starting process");
    m_ExpectedRunningState = kStarted;
    setSynergyState(synergyConnecting);

    QString app;
    QStringList args;

    args << "-f" << "--no-tray" << "--debug" << appConfig().logLevelText();


    args << "--name" << getScreenName();

    if (desktopMode)
    {
        setSynergyProcess(new QProcess(this));
    }
    else
    {
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
        if (appConfig().elevateMode() == ElevateAsNeeded) {
                args << "--stop-on-desk-switch";
        }
#endif
    }

#ifndef Q_OS_LINUX

    if (m_ServerConfig.enableDragAndDrop()) {
        args << "--enable-drag-drop";
    }

#endif

#if defined(Q_OS_WIN)
    if (m_AppConfig->getCryptoEnabled()) {
        args << "--enable-crypto";
        args << "--tls-cert" <<  QString("\"%1\"").arg(m_AppConfig->getTLSCertPath());
    }
    // on windows, the profile directory changes depending on the user that
    // launched the process (e.g. when launched with elevation). setting the
    // profile dir on launch ensures it uses the same profile dir is used
    // no matter how its relaunched.
    args << "--profile-dir" << getProfileRootForArg();

#else
    if (m_AppConfig->getCryptoEnabled()) {
        args << "--enable-crypto";
        args << "--tls-cert" << m_AppConfig->getTLSCertPath();
    }
#endif

    if ((synergyType() == synergyClient && !clientArgs(args, app))
        || (synergyType() == synergyServer && !serverArgs(args, app)))
    {
        stopSynergy();
        return;
    }

    if (desktopMode)
    {
        connect(synergyProcess(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(synergyFinished(int, QProcess::ExitStatus)));
        connect(synergyProcess(), SIGNAL(readyReadStandardOutput()), this, SLOT(logOutput()));
        connect(synergyProcess(), SIGNAL(readyReadStandardError()), this, SLOT(logError()));
    }

    // put a space between last log output and new instance.
    if (!m_pLogOutput->toPlainText().isEmpty())
        appendLogRaw("");

    appendLogInfo("starting " + QString(synergyType() == synergyServer ? "server" : "client"));

    qDebug() << args;

    // show command if debug log level...
    if (appConfig().logLevel() >= 4) {
        appendLogInfo(QString("command: %1 %2").arg(app, args.join(" ")));
    }

    appendLogInfo("config file: " + configFilename());
    appendLogInfo("log level: " + appConfig().logLevelText());

    if (appConfig().logToFile())
        appendLogInfo("log file: " + appConfig().logFilename());

    if (desktopMode)
    {
        synergyProcess()->start(app, args);
        if (!synergyProcess()->waitForStarted())
        {
            show();
            QMessageBox::warning(this, tr("Program can not be started"), QString(tr("The executable<br><br>%1<br><br>could not be successfully started, although it does exist. Please check if you have sufficient permissions to run this program.").arg(app)));
            return;
        }
    }

    if (serviceMode)
    {
        QString command(app + " " + args.join(" "));
        m_IpcClient.sendCommand(command, appConfig().elevateMode());
    }
}

void MainWindow::retryStart()
{
    //This function is only called after a failed start
    //Only start synergy if the current state is pending retry
    if (m_SynergyState == synergyPendingRetry)
    {
        startSynergy();
    }
}

bool MainWindow::clientArgs(QStringList& args, QString& app)
{
    app = appPath(appConfig().synergycName());

    if (!QFile::exists(app))
    {
        show();
        QMessageBox::warning(this, tr("Synergy client not found"),
                             tr("The executable for the synergy client does not exist."));
        return false;
    }

#if defined(Q_OS_WIN)
    // wrap in quotes so a malicious user can't start \Program.exe as admin.
    app = QString("\"%1\"").arg(app);
#endif

    if (appConfig().logToFile())
    {
        appConfig().persistLogDir();
        args << "--log" << appConfig().logFilenameCmd();
    }

#ifndef SYNERGY_ENTERPRISE
    // check auto config first, if it is disabled or no server detected,
    // use line edit host name if it is not empty
    if (appConfig().autoConfig()) {
        if (m_pComboServerList->count() != 0) {
            QString serverIp = m_pComboServerList->currentText();
            args << serverIp + ":" + QString::number(appConfig().port());
            return true;
        }
        else {
            show();
            QMessageBox::warning(
                this, tr("No server selected"),
                tr("No auto config server was selected, try manual mode instead."));
            return false;
        }
    }
#endif

    if (m_pLineEditHostname->text().isEmpty())
    {
#ifndef SYNERGY_ENTERPRISE
        //check if autoconfig mode is enabled
        if (!appConfig().autoConfig())
        {
#endif
            show();
            QMessageBox::warning(
                this, tr("Hostname is empty"),
                tr("Please fill in a hostname for the synergy client to connect to."));
            return false;

#ifndef SYNERGY_ENTERPRISE
        }
        else
        {
            return false;
        }
#endif
    }
    args << m_pLineEditHostname->text() + ":" + QString::number(appConfig().port());
    return true;
}

QString MainWindow::configFilename()
{
    QString filename;
    if (m_pRadioInternalConfig->isChecked())
    {
        // TODO: no need to use a temporary file, since we need it to
        // be permenant (since it'll be used for Windows services, etc).
        QTemporaryFile tempConfigFile;
        tempConfigFile.setAutoRemove(false);

        if (!tempConfigFile.open())
        {
            QMessageBox::critical(this, tr("Cannot write configuration file"), tr("The temporary configuration file required to start synergy can not be written."));
            return "";
        }

        serverConfig().save(tempConfigFile);
        filename = tempConfigFile.fileName();

        tempConfigFile.close();
    }
    else
    {
        if (!QFile::exists(m_pLineEditConfigFile->text()))
        {
            if (QMessageBox::warning(this, tr("Configuration filename invalid"),
                tr("You have not filled in a valid configuration file for the synergy server. "
                        "Do you want to browse for the configuration file now?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes
                    || !on_m_pButtonBrowseConfigFile_clicked())
                return "";
        }

        filename = m_pLineEditConfigFile->text();
    }
    return filename;
}

QString MainWindow::address()
{
    QString i = appConfig().networkInterface();
    return (!i.isEmpty() ? i : "") + ":" + QString::number(appConfig().port());
}

QString MainWindow::appPath(const QString& name)
{
    return appConfig().synergyProgramDir() + name;
}

bool MainWindow::serverArgs(QStringList& args, QString& app)
{
    app = appPath(appConfig().synergysName());

    if (!QFile::exists(app))
    {
        QMessageBox::warning(this, tr("Synergy server not found"),
                             tr("The executable for the synergy server does not exist."));
        return false;
    }

#if defined(Q_OS_WIN)
    // wrap in quotes so a malicious user can't start \Program.exe as admin.
    app = QString("\"%1\"").arg(app);
#endif

    if (appConfig().logToFile())
    {
        appConfig().persistLogDir();

        args << "--log" << appConfig().logFilenameCmd();
    }

    QString configFilename = this->configFilename();
#if defined(Q_OS_WIN)
    // wrap in quotes in case username contains spaces.
    configFilename = QString("\"%1\"").arg(configFilename);
#endif
    args << "-c" << configFilename << "--address" << address();

#ifndef SYNERGY_ENTERPRISE
    if (!appConfig().serialKey().isEmpty()) {
        args << "--serial-key" << appConfig().serialKey();
    }
#endif

    return true;
}

void MainWindow::stopSynergy()
{
    appendLogDebug("stopping process");

    m_ExpectedRunningState = kStopped;

    if (appConfig().processMode() == Service)
    {
        stopService();
    }
    else if (appConfig().processMode() == Desktop)
    {
        stopDesktop();
    }

    setSynergyState(synergyDisconnected);

    // reset so that new connects cause auto-hide.
    m_AlreadyHidden = false;
}

void MainWindow::stopService()
{
    // send empty command to stop service from laucning anything.
    m_IpcClient.sendCommand("", appConfig().elevateMode());
}

void MainWindow::stopDesktop()
{
    QMutexLocker locker(&m_StopDesktopMutex);
    if (!synergyProcess()) {
        return;
    }

    appendLogInfo("stopping synergy desktop process");

    if (synergyProcess()->isOpen()) {
        synergyProcess()->close();
    }

    delete synergyProcess();
    setSynergyProcess(NULL);
}

void MainWindow::synergyFinished(int exitCode, QProcess::ExitStatus)
{
    if (exitCode == 0) {
        appendLogInfo(QString("process exited normally"));
    }
    else {
        appendLogError(QString("process exited with error code: %1").arg(exitCode));
    }

    if (m_ExpectedRunningState == kStarted) {

        setSynergyState(synergyPendingRetry);
        QTimer::singleShot(1000, this, SLOT(retryStart()));
        appendLogInfo(QString("detected process not running, auto restarting"));
    }
    else {
        setSynergyState(synergyDisconnected);
    }
}

void MainWindow::setSynergyState(qSynergyState state)
{
    // always assume connection is not secure when connection changes
    // to anything except connected. the only way the padlock shows is
    // when the correct TLS version string is detected.
    if (state != synergyConnected) {
        secureSocket(false);
    }

    if (synergyState() == state)
        return;

    if ((state == synergyConnected) || (state == synergyConnecting) || (state == synergyListening) || (state == synergyPendingRetry))
    {
        disconnect (m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStartSynergy, SLOT(trigger()));
        connect (m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStopSynergy, SLOT(trigger()));
        m_pButtonToggleStart->setText(tr("&Stop"));
        m_pButtonApply->setEnabled(true);
    }
    else if (state == synergyDisconnected)
    {
        disconnect (m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStopSynergy, SLOT(trigger()));
        connect (m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStartSynergy, SLOT(trigger()));
        m_pButtonToggleStart->setText(tr("&Start"));
        m_pButtonApply->setEnabled(false);
    }

    bool running = false;
    if (state == synergyConnected || state == synergyListening) {
        running = true;
    }

    m_pActionStartSynergy->setEnabled(!running);
    m_pActionStopSynergy->setEnabled(running);

    switch (state)
    {
    case synergyListening: {
        if (synergyType() == synergyServer) {
            setStatus(tr("Synergy is waiting for clients").arg(m_SecureSocketVersion));
        }

        break;
    }
    case synergyConnected: {
        if (m_SecureSocket) {
            setStatus(tr("Synergy is connected (with %1)").arg(m_SecureSocketVersion));
        }
        else {
            setStatus(tr("Synergy is running (without TLS encryption)").arg(m_SecureSocketVersion));
        }
        break;
    }
    case synergyConnecting:
        setStatus(tr("Synergy is starting..."));
        break;
    case synergyPendingRetry:
        setStatus(tr("There was an error, retrying..."));
        break;
    case synergyDisconnected:
        setStatus(tr("Synergy is not running"));
        break;
    }

    setIcon(state);

    m_SynergyState = state;
}

void MainWindow::setVisible(bool visible)
{
    QMainWindow::setVisible(visible);
    m_pActionMinimize->setEnabled(visible);
    m_pActionRestore->setEnabled(!visible);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 // lion
    // dock hide only supported on lion :(
    ProcessSerialNumber psn = { 0, kCurrentProcess };
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    GetCurrentProcess(&psn);
#pragma GCC diagnostic pop
    if (visible)
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
    else
        TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
#endif
}

QString MainWindow::getIPAddresses()
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

    bool hinted = false;
    QString result;
    for (int i = 0; i < addresses.size(); i++) {
        if (addresses[i].protocol() == QAbstractSocket::IPv4Protocol &&
            addresses[i] != QHostAddress(QHostAddress::LocalHost)) {

            QString address = addresses[i].toString();
            QString format = "%1, ";

            // usually 192.168.x.x is a useful ip for the user, so indicate
            // this by making it bold.
            if (!hinted && address.startsWith("192.168")) {
                hinted = true;
                format = "<b>%1</b>, ";
            }
            //Prevent self assigned IPs being displayed
            if (!address.startsWith("169.254")) {
                result += format.arg(address);
            }
        }
    }

    if (result == "") {
        return tr("Unknown");
    }

    // remove trailing comma.
    result.chop(2);

    return result;
}

QString MainWindow::getScreenName()
{
    if (appConfig().screenName() == "") {
        return QHostInfo::localHostName();
    }
    else {
        return appConfig().screenName();
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event != 0)
    {
        switch (event->type())
        {
        case QEvent::LanguageChange:
        {
            retranslateUi(this);
            retranslateMenuBar();

            proofreadInfo();

            break;
        }
        case QEvent::WindowStateChange:
        {
            windowStateChanged();
            break;
        }
        }
    }
    // all that do not return are allowing the event to propagate
    QMainWindow::changeEvent(event);
}

void MainWindow::addZeroconfServer(const QString name)
{
    // don't add yourself to the server list.
    if (getIPAddresses().contains(name)) {
        return;
    }

    if (m_pComboServerList->findText(name) == -1) {
        m_pComboServerList->addItem(name);
    }
}

void MainWindow::setEdition(Edition edition)
{
#ifndef SYNERGY_ENTERPRISE
    setWindowTitle(m_LicenseManager->getEditionName (edition));
#endif
}

#ifndef SYNERGY_ENTERPRISE
void MainWindow::InvalidLicense()
{
   stopSynergy();
   m_AppConfig->activationHasRun(false);
}

void MainWindow::showLicenseNotice(const QString& notice)
{
    this->m_trialWidget->hide();

    if (!notice.isEmpty()) {
        this->m_trialLabel->setText(notice);
        this->m_trialWidget->show();
    }

    setWindowTitle (m_LicenseManager->activeEditionName());
}
#endif

void MainWindow::updateLocalFingerprint()
{
    if (m_AppConfig->getCryptoEnabled() && Fingerprint::local().fileExists()) {
        m_pLabelFingerprint->setVisible(true);
        m_pLabelLocalFingerprint->setVisible(true);
        m_pLabelLocalFingerprint->setText(Fingerprint::local().readFirst());
    }
    else {
        m_pLabelFingerprint->setVisible(false);
        m_pLabelLocalFingerprint->setVisible(false);
    }
}

#ifndef SYNERGY_ENTERPRISE
LicenseManager&
MainWindow::licenseManager() const
{
    return *m_LicenseManager;
}
#endif

void MainWindow::on_m_pGroupClient_toggled(bool on)
{
    m_pGroupServer->setChecked(!on);

    // only call in either client or server toggle, but not both
    // since the toggle functions call eachother indirectly.
    updateZeroconfService();
}

void MainWindow::on_m_pGroupServer_toggled(bool on)
{
    m_pGroupClient->setChecked(!on);
}

bool MainWindow::on_m_pButtonBrowseConfigFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Browse for a synergys config file"), QString(), synergyConfigFilter);

    if (!fileName.isEmpty())
    {
        m_pLineEditConfigFile->setText(fileName);
        return true;
    }

    return false;
}

bool  MainWindow::on_m_pActionSave_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save configuration as..."));

    if (!fileName.isEmpty() && !serverConfig().save(fileName))
    {
        QMessageBox::warning(this, tr("Save failed"), tr("Could not save configuration to file."));
        return true;
    }

    return false;
}

void MainWindow::on_m_pActionAbout_triggered()
{
    AboutDialog dlg(this, appPath(appConfig().synergycName()));
    dlg.exec();
}

void MainWindow::on_m_pActionHelp_triggered()
{
    QDesktopServices::openUrl(QUrl(HELP_URL));
}

void MainWindow::updateZeroconfService()
{
#ifndef SYNERGY_ENTERPRISE

    // reset the server list in case one has gone away.
    // it'll be re-added after the zeroconf service restarts.
    m_pComboServerList->clear();

    if (m_pZeroconf != nullptr) {
        if (appConfig().autoConfig()) {
            m_pZeroconf->startService();
        }
        else {
            m_pZeroconf->stopService();
        }
    }
#endif
}

void MainWindow::updateAutoConfigWidgets()
{
    if (appConfig().autoConfig()) {
        m_pLabelAutoDetected->show();
        m_pComboServerList->show();

        m_pLabelServerName->hide();
        m_pLineEditHostname->hide();

        m_pWidgetAutoConfig->hide();
    }
    else {
        m_pLabelServerName->show();
        m_pLineEditHostname->show();

        m_pLabelAutoDetected->hide();
        m_pComboServerList->hide();

#ifndef SYNERGY_ENTERPRISE
        m_pWidgetAutoConfig->show();
#else
        m_pWidgetAutoConfig->hide();
#endif
    }
}

void MainWindow::on_m_pActionSettings_triggered()
{
    SettingsDialog(this, appConfig()).exec();
}

void MainWindow::autoAddScreen(const QString name)
{
    if (m_ServerConfig.ignoreAutoConfigClient()) {
        appendLogDebug(QString("ignoring zeroconf screen: %1").arg(name));
        return;
    }

#ifndef SYNERGY_ENTERPRISE
    if (m_ActivationDialogRunning) {
        // TODO: refactor this code
        // add this screen to the pending list and check this list until
        // users finish activation dialog
        m_PendingClientNames.append(name);
        return;
    }
#endif

    int r = m_ServerConfig.autoAddScreen(name);
    if (r != kAutoAddScreenOk) {
        switch (r) {
        case kAutoAddScreenManualServer:
            showConfigureServer(
                tr("Please add the server (%1) to the grid.")
                    .arg(appConfig().screenName()));
            break;

        case kAutoAddScreenManualClient:
            showConfigureServer(
                tr("Please drag the new client screen (%1) "
                    "to the desired position on the grid.")
                    .arg(name));
            break;
        }
    }
}

void MainWindow::showConfigureServer(const QString& message)
{
    ServerConfigDialog dlg(this, serverConfig(), appConfig().screenName());
    dlg.message(message);
    dlg.exec();
}

void MainWindow::on_m_pButtonConfigureServer_clicked()
{
    showConfigureServer();
}

void MainWindow::on_m_pActivate_triggered()
{
#ifndef SYNERGY_ENTERPRISE
    raiseActivationDialog();
#endif
}

void MainWindow::on_m_pButtonApply_clicked()
{
    restartSynergy();
}

#ifndef SYNERGY_ENTERPRISE
int MainWindow::raiseActivationDialog()
{
    if (m_ActivationDialogRunning) {
        return QDialog::Rejected;
    }
    ActivationDialog activationDialog (this, appConfig(), licenseManager());
    m_ActivationDialogRunning = true;
    connect (&activationDialog, SIGNAL(finished(int)),
             this, SLOT(on_activationDialogFinish()), Qt::QueuedConnection);
    int result = activationDialog.exec();
    m_ActivationDialogRunning = false;
    if (!m_PendingClientNames.empty()) {
        foreach (const QString& name, m_PendingClientNames) {
            autoAddScreen(name);
        }

        m_PendingClientNames.clear();
    }
    return result;
}
#endif

void MainWindow::on_windowShown()
{
#ifndef SYNERGY_ENTERPRISE
	if (!m_AppConfig->activationHasRun() &&
		!m_LicenseManager->serialKey().isValid()){
			raiseActivationDialog();
	}
#endif
}

QString MainWindow::getProfileRootForArg()
{
    CoreInterface coreInterface;
    QString dir = coreInterface.getProfileDir();

    // HACK: strip our app name since we're returning the root dir.
#if defined(Q_OS_WIN)
    dir.replace("\\Synergy", "");
#else
    dir.replace("/.synergy", "");
#endif

    return QString("\"%1\"").arg(dir);
}

void MainWindow::secureSocket(bool secureSocket)
{
    m_SecureSocket = secureSocket;
    if (secureSocket) {
        m_pLabelPadlock->show();
    }
    else {
        m_pLabelPadlock->hide();
    }
}

void MainWindow::on_m_pLabelAutoConfig_linkActivated(const QString &)
{
    m_pActionSettings->trigger();
}

void MainWindow::on_m_pComboServerList_currentIndexChanged(const QString &server)
{
    appConfig().setAutoConfigServer(server);
}

void MainWindow::windowStateChanged()
{
    if (windowState() == Qt::WindowMinimized && appConfig().getMinimizeToTray())
        hide();
}

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

#include <array>

#include "MainWindow.h"

#include "Fingerprint.h"
#include "AboutDialog.h"
#include "AboutDialogEliteBackers.h"
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
    m_ServerConfig(5, 3, m_AppConfig, this),
    m_AlreadyHidden(false),
    m_pMenuBar(NULL),
    m_pMenuFile(NULL),
    m_pMenuEdit(NULL),
    m_pMenuWindow(NULL),
    m_pMenuHelp(NULL),
    m_pCancelButton(NULL),
    m_ExpectedRunningState(kStopped),
    m_SecureSocket(false),
    m_serverConnection(*this),
    m_clientConnection(*this)
{
#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)
    m_pZeroconf = new Zeroconf(this);
#endif

    setupUi(this);

#if defined(Q_OS_MAC)
    m_pRadioGroupServer->setAttribute(Qt::WA_MacShowFocusRect,0);
    m_pRadioGroupClient->setAttribute(Qt::WA_MacShowFocusRect,0);
#endif

    updateAutoConfigWidgets();

    createMenuBar();
    loadSettings();
    initConnections();

    m_pWidgetUpdate->hide();
    m_VersionChecker.setApp(appPath(appConfig.synergycName()));

    updateScreenName();
    connect(m_AppConfig, SIGNAL(screenNameChanged()), this, SLOT(updateScreenName()));
    m_pLabelIpAddresses->setText(tr("This computer's IP addresses: %1").arg(getIPAddresses()));

#if defined(Q_OS_WIN)
    // ipc must always be enabled, so that we can disable command when switching to desktop mode.
    connect(&m_IpcClient, SIGNAL(readLogLine(const QString&)), this, SLOT(appendLogRaw(const QString&)));
    connect(&m_IpcClient, SIGNAL(errorMessage(const QString&)), this, SLOT(appendLogError(const QString&)));
    connect(&m_IpcClient, SIGNAL(infoMessage(const QString&)), this, SLOT(appendLogInfo(const QString&)));
    connect(&m_IpcClient, SIGNAL(readLogLine(const QString&)), this, SLOT(handleIdleService(const QString&)));
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

    m_trialLabel->hide();

    // hide padlock icon
    secureSocket(false);



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

    updateWindowTitle();

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

#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)
    updateZeroconfService();

    addZeroconfServer(m_AppConfig->autoConfigServer());

    updateAutoConfigWidgets();
#endif
}

MainWindow::~MainWindow()
{
    if (appConfig().processMode() == Desktop) {
        m_ExpectedRunningState = kStopped;
        stopDesktop();
    }

#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)
    delete m_pZeroconf;
#endif
}

void MainWindow::open()
{
    std::array<QAction *, 7> trayMenu = {
        m_pActionStartSynergy,
        m_pActionStopSynergy,
        nullptr,
        m_pActionMinimize,
        m_pActionRestore,
        nullptr,
        m_pActionQuit
    };

    m_trayIcon.create(trayMenu, [this](QObject const *o, const char *s) {
        connect(o, s, this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
        setIcon(synergyDisconnected);
    });

    if (appConfig().getAutoHide()) {
        hide();
    } else {
        showNormal();
    }

    m_VersionChecker.checkLatest();

    // only start if user has previously started. this stops the gui from
    // auto hiding before the user has configured synergy (which of course
    // confuses first time users, who think synergy has crashed).
    if (appConfig().startedBefore() && appConfig().processMode() == ProcessMode::Desktop) {
        startSynergy();
    }
}

void MainWindow::setStatus(const QString &status)
{
    m_pStatusLabel->setText(status);
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
    enableServer(appConfig().getServerGroupChecked());
    enableClient(appConfig().getClientGroupChecked());

    m_pLineEditHostname->setText(appConfig().getServerHostname());
    m_pLineEditClienIp->setText(serverConfig().getClientAddress());
}

void MainWindow::initConnections()
{
    connect(m_pActionMinimize, SIGNAL(triggered()), this, SLOT(hide()));
    connect(m_pActionRestore, SIGNAL(triggered()), this, SLOT(showNormal()));
    connect(m_pActionStartSynergy, SIGNAL(triggered()), this, SLOT(actionStart()));
    connect(m_pActionStopSynergy, SIGNAL(triggered()), this, SLOT(stopSynergy()));
    connect(m_pActionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(&m_VersionChecker, SIGNAL(updateFound(const QString&)), this, SLOT(updateFound(const QString&)));
}

void MainWindow::saveSettings()
{
    // program settings
    appConfig().setServerGroupChecked(m_pRadioGroupServer->isChecked());
    appConfig().setClientGroupChecked(m_pRadioGroupClient->isChecked());
    appConfig().setServerHostname(m_pLineEditHostname->text());
    serverConfig().setClientAddress(m_pLineEditClienIp->text());

    /* Save everything */
    GUI::Config::ConfigWriter::make()->globalSave();
}

void MainWindow::zeroConfToggled() {
#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)
    updateZeroconfService();

    addZeroconfServer(m_AppConfig->autoConfigServer());

    updateAutoConfigWidgets();
#endif
}

void MainWindow::setIcon(qSynergyState state) const
{
    QIcon icon;

#ifdef Q_OS_MAC
    switch(getOSXIconsTheme()) {
        case IconsTheme::ICONS_DARK:
            icon.addFile(synergyDarkIconFiles[state]);
            break;
        case IconsTheme::ICONS_LIGHT:
            icon.addFile(synergyLightIconFiles[state]);
            break;
        case IconsTheme::ICONS_TEMPLATE:
        default:
            icon.addFile(synergyDarkIconFiles[state]);
            icon.setIsMask(true);
            break;
    }
#else
    icon.addFile(synergyDefaultIconFiles[state]);
#endif

    m_trayIcon.set(icon);
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

void MainWindow::handleIdleService(const QString& text)
{
    foreach(QString line, text.split(QRegExp("\r|\n|\r\n"))) {
        // only start if there is no active service running
        if (!line.isEmpty() && line.contains("service status: idle") && appConfig().startedBefore()) {
            startSynergy();
        }
    }
}

void MainWindow::updateFromLogLine(const QString &line)
{
    // TODO: This shouldn't be updating from log needs a better way of doing this
    checkConnected(line);
    checkFingerprint(line);
    checkSecureSocket(line);

    // subprocess (synergys, synergyc) is not allowed to show notifications
    // process the log from it and show notificatino from synergy instead
#ifdef Q_OS_MAC
    checkOSXNotification(line);
#endif

#ifndef SYNERGY_ENTERPRISE
    checkLicense(line);
#endif
}

void MainWindow::checkConnected(const QString& line)
{
    // TODO: implement ipc connection state messages to replace this hack.
    if (m_pRadioGroupServer->isChecked())
    {
        m_serverConnection.update(line);
        m_pLabelServerState->updateServerState(line);
    }
    else
    {
        m_clientConnection.update(line);
        m_pLabelClientState->updateClientState(line);
    }

    if (line.contains("connected to server") || line.contains("has connected"))
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
        m_SecureSocketVersion = line.mid(index + strlen(tlsCheckString)); // Compliant: we made sure that tlsCheckString variable ended with null(static const char* declaration)
    }
}

#ifdef Q_OS_MAC
void MainWindow::checkOSXNotification(const QString& line)
{
    static const QString OSXNotificationSubstring = "OSX Notification: ";
    if (line.contains(OSXNotificationSubstring) && line.contains('|')) {
        int delimterPosition = line.indexOf('|');
        int notificationStartPosition = line.indexOf(OSXNotificationSubstring);
        QString title =
                line.mid(notificationStartPosition + OSXNotificationSubstring.length(),
                         delimterPosition - notificationStartPosition - OSXNotificationSubstring.length());
        QString body =
                line.mid(delimterPosition + 1,
                         line.length() - delimterPosition);
        if (!showOSXNotification(title, body)) {
            appendLogInfo("OSX notification was not shown");
        }
    }
}
#endif

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
    saveSettings();

#ifdef Q_OS_MAC
    requestOSXNotificationPermission();
#endif

#ifndef SYNERGY_ENTERPRISE
    SerialKey serialKey = m_LicenseManager->serialKey();
    if (!serialKey.isValid()) {
        if (QDialog::Rejected == raiseActivationDialog()) {
            return;
        }
    }
    m_LicenseManager->registerLicense();
#endif
    bool desktopMode = appConfig().processMode() == Desktop;
    bool serviceMode = appConfig().processMode() == Service;

    appendLogDebug("starting process");
    m_ExpectedRunningState = kStarted;
    setSynergyState(synergyConnecting);

    QString app;
    QStringList args;

    args << "-f" << "--no-tray" << "--debug" << appConfig().logLevelText();


    args << "--name" << appConfig().screenName();

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

    if (m_AppConfig->getPreventSleep())
    {
        args << "--prevent-sleep";
    }

    // put a space between last log output and new instance.
    if (!m_pLogOutput->toPlainText().isEmpty())
        appendLogRaw("");

    appendLogInfo("starting " + QString(synergyType() == synergyServer ? "server" : "client"));

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

    qDebug() << args;

    // show command if debug log level...
    if (appConfig().logLevel() >= 4) {
        appendLogInfo(QString("command: %1 %2").arg(app, args.join(" ")));
    }

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

void MainWindow::actionStart()
{
    m_clientConnection.setCheckConnection(true);
    startSynergy();
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

    if (appConfig().getLanguageSync()) {
        args << "--sync-language";
    }

    if (appConfig().getInvertScrollDirection()) {
        args <<"--invert-scroll";
    }

#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)
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

    if (m_pLineEditHostname->text().isEmpty() && !appConfig().getClientHostMode())
    {
#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)
        //check if autoconfig mode is enabled
        if (!appConfig().autoConfig())
        {
#endif
            show();
            QMessageBox::warning(
                this, tr("Hostname is empty"),
                tr("Please fill in a hostname for the synergy client to connect to."));
            return false;

#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)
        }
        else
        {
            return false;
        }
#endif
    }

    if (appConfig().getClientHostMode()) {
        args <<"--host";
        args <<":" + QString::number(appConfig().port());
    }
    else {
        QString hostName = m_pLineEditHostname->text();
        // if interface is IPv6 - ensure that ip is in square brackets
        if (hostName.count(':') > 1) {
            if(hostName[0] != '[') {
                hostName.insert(0, '[');
            }
            if(hostName[hostName.size() - 1] != ']') {
                hostName.push_back(']');
            }
        }
        args << hostName + ":" + QString::number(appConfig().port());
    }

    return true;
}

QString MainWindow::configFilename()
{
    QString configFullPath;
    if (appConfig().getUseExternalConfig())
    {
        configFullPath = appConfig().getConfigFile();
    }
    else
    {
        QStringList errors;
        for (auto path : {QStandardPaths::AppDataLocation,
                          QStandardPaths::AppConfigLocation})
        {
            auto configDirPath = QStandardPaths::writableLocation(path);
            if (!QDir().mkpath(configDirPath))
            {
                errors.push_back(tr("Failed to create config folder \"%1\"").arg(configDirPath));
                continue;
            }

            QFile configFile(configDirPath + "/LastConfig.cfg");
            if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                errors.push_back(tr("File:\"%1\" Error:%2").arg(configFile.fileName(), configFile.errorString()));
                continue;
            }

            serverConfig().save(configFile);
            configFile.close();
            configFullPath = configFile.fileName();

            break;
        }

        if (configFullPath.isEmpty())
        {
            QMessageBox::critical(this, tr("Cannot write configuration file"), errors.join('\n'));
        }
    }

    return configFullPath;
}

QString MainWindow::address() const
{
    QString i = appConfig().networkInterface();
    // if interface is IPv6 - ensure that ip is in square brackets
    if (i.count(':') > 1) {
        if(i[0] != '[') {
            i.insert(0, '[');
        }
        if(i[i.size() - 1] != ']') {
            i.push_back(']');
        }
    }
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

    if (appConfig().getServerClientMode() &&
        m_pLineEditClienIp->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Client IP address or name is empty"),
                             tr("Please fill in a client IP address or name."));
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
    if (configFilename.isEmpty())
    {
        return false;
    }
#if defined(Q_OS_WIN)
    // wrap in quotes in case username contains spaces.
    configFilename = QString("\"%1\"").arg(configFilename);
#endif
    args << "-c" << configFilename << "--address" << address();
    appendLogInfo("config file: " + configFilename);

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
    QStringList result;
    bool hinted = false;
    const auto localnet = QHostAddress::parseSubnet("192.168.0.0/16");
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

    for (const auto& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress(QHostAddress::LocalHost) &&
            !address.isInSubnet(QHostAddress::parseSubnet("169.254.0.0/16"))) {

            // usually 192.168.x.x is a useful ip for the user, so indicate
            // this by making it bold.
            if (!hinted && address.isInSubnet(localnet)) {
                QString format = "<span style=\"color:#4285F4;\">%1</span>";
                result.append(format.arg(address.toString()));
                hinted = true;
            }
            else {
                result.append(address.toString());
            }
        }
    }

    if (result.isEmpty()) {
        result.append(tr("Unknown"));
    }

    return result.join(", ");
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
            updateWindowTitle();
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
    this->m_trialLabel->hide();

    if (!notice.isEmpty()) {
        this->m_trialLabel->setText(notice);
        this->m_trialLabel->show();
    }

    setWindowTitle (m_LicenseManager->activeEditionName());
}
#endif

void MainWindow::updateLocalFingerprint()
{
    if (m_AppConfig->getCryptoEnabled() &&
        Fingerprint::local().fileExists() &&
        m_pRadioGroupServer->isChecked())
    {
        m_pLabelFingerprint->setVisible(true);
    }
    else
    {
        m_pLabelFingerprint->setVisible(false);
    }
}

#ifndef SYNERGY_ENTERPRISE
LicenseManager&
MainWindow::licenseManager() const
{
    return *m_LicenseManager;
}
#endif

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
#if defined(SYNERGY_ENTERPRISE) || defined(SYNERGY_BUSINESS)
    AboutDialog dlg(this, appConfig());
    dlg.exec();
#else
    if (appConfig().edition() == Edition::kBusiness) {
        AboutDialog dlg(this, appConfig());
        dlg.exec();
    }
    else {
        AboutDialogEliteBackers dlg(this, appConfig());
        dlg.exec();
    }
#endif
}

void MainWindow::on_m_pActionHelp_triggered()
{
    QDesktopServices::openUrl(QUrl(HELP_URL));
}

void MainWindow::updateZeroconfService()
{
#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)

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
   m_pLabelServerName->show();
   m_pLineEditHostname->show();

   m_pLabelAutoDetected->hide();
   m_pComboServerList->hide();
}

void MainWindow::updateWindowTitle()
{
#ifdef SYNERGY_ENTERPRISE
    setWindowTitle ("Synergy 1 Enterprise");
#else
    setWindowTitle (m_LicenseManager->activeEditionName());
    m_LicenseManager->refresh();
#endif
}

void MainWindow::on_m_pActionSettings_triggered()
{
    auto result = SettingsDialog(this, appConfig()).exec();
    if(result == QDialog::Accepted)
    {
        enableServer(appConfig().getServerGroupChecked());
        enableClient(appConfig().getClientGroupChecked());
        auto state = synergyState();
        if ((state == synergyConnected) || (state == synergyConnecting) || (state == synergyListening)) {
            restartSynergy();
        }
    }
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
    ServerConfigDialog dlg(this, serverConfig());
    dlg.message(message);
    auto result = dlg.exec();

    if(result == QDialog::Accepted) {
        auto state = synergyState();
        if ((state == synergyConnected) || (state == synergyConnecting) || (state == synergyListening)) {
            restartSynergy();
        }
    }
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
    m_clientConnection.setCheckConnection(true);
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
    auto serialKey = m_LicenseManager->serialKey();
    if (!m_AppConfig->activationHasRun() && !serialKey.isValid()) {
        setEdition(Edition::kUnregistered);
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

void MainWindow::on_m_pLabelComputerName_linkActivated(const QString&)
{
   m_pActionSettings->trigger();
}

void MainWindow::on_m_pLabelFingerprint_linkActivated(const QString&)
{
    QMessageBox::information(this, "SSL/TLS fingerprint", Fingerprint::local().readFirst());
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

void MainWindow::updateScreenName()
{
    m_pLabelComputerName->setText(tr("This computer's name: %1 (<a href=\"#\" style=\"text-decoration: none; color: #4285F4;\">Preferences</a>)").arg(appConfig().screenName()));
    serverConfig().updateServerName();
}

void MainWindow::enableServer(bool enable)
{
    m_AppConfig->setServerGroupChecked(enable);
    m_pRadioGroupServer->setChecked(enable);

    if (enable)
    {
        if (m_AppConfig->getServerClientMode()) {
            m_pLabelClientIp->show();
            m_pLineEditClienIp->show();
            m_pButtonConnectToClient->show();
        }
        else {
            m_pLabelClientIp->hide();
            m_pLineEditClienIp->hide();
            m_pButtonConnectToClient->hide();
        }

        m_pButtonConfigureServer->show();
        m_pLabelServerState->show();
        updateLocalFingerprint();
        m_pButtonToggleStart->setEnabled(enable);
    }
    else
    {
        m_pLabelFingerprint->hide();
        m_pButtonConfigureServer->hide();
        m_pLabelServerState->hide();
        m_pLabelClientIp->hide();
        m_pLineEditClienIp->hide();
        m_pButtonConnectToClient->hide();
    }
}

void MainWindow::enableClient(bool enable)
{
    m_AppConfig->setClientGroupChecked(enable);
    m_pRadioGroupClient->setChecked(enable);

    if (enable)
    {
        if (m_AppConfig->getClientHostMode())
        {
            m_pLabelServerName->hide();
            m_pLineEditHostname->hide();
            m_pButtonConnect->hide();
        }
        else
        {
            m_pLabelServerName->show();
            m_pLineEditHostname->show();
            m_pButtonConnect->show();
        }
        m_pButtonToggleStart->setEnabled(enable);
    }
    else
    {
        m_pLabelClientState->hide();
        m_pLabelServerName->hide();
        m_pLineEditHostname->hide();
        m_pButtonConnect->hide();
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
#if defined (Q_OS_LINUX)
    QCoreApplication::quit();
#endif
    QWidget::closeEvent(event);
}

void MainWindow::on_m_pRadioGroupServer_clicked(bool)
{
    enableServer(true);
    enableClient(false);
    m_AppConfig->saveSettings();
}

void MainWindow::on_m_pRadioGroupClient_clicked(bool)
{
    enableClient(true);
    enableServer(false);
    m_AppConfig->saveSettings();
}

void MainWindow::on_m_pButtonConnect_clicked()
{
    on_m_pButtonApply_clicked();
}

void MainWindow::on_m_pButtonConnectToClient_clicked()
{
    on_m_pButtonApply_clicked();
}


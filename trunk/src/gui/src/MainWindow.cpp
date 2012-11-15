/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define WEBSITE_ADDRESS "synergy-foss.org"

#include <iostream>

#include "MainWindow.h"
#include "AboutDialog.h"
#include "ServerConfigDialog.h"
#include "SettingsDialog.h"
#include "SetupWizard.h"

#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include <QNetworkAccessManager>

#if defined(Q_OS_MAC)
#include <ApplicationServices/ApplicationServices.h>
#endif

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#if defined(Q_OS_WIN)
static const char synergyConfigName[] = "synergy.sgc";
static const QString synergyConfigFilter(QObject::tr("Synergy Configurations (*.sgc);;All files (*.*)"));
#else
static const char synergyConfigName[] = "synergy.conf";
static const QString synergyConfigFilter(QObject::tr("Synergy Configurations (*.conf);;All files (*.*)"));
#endif

static const char* synergyIconFiles[] =
{
	":/res/icons/16x16/synergy-disconnected.png",
	":/res/icons/16x16/synergy-disconnected.png",
	":/res/icons/16x16/synergy-connected.png"
};

MainWindow::MainWindow(QSettings& settings, AppConfig& appConfig) :
	m_Settings(settings),
	m_AppConfig(appConfig),
	m_pSynergy(NULL),
	m_SynergyState(synergyDisconnected),
	m_ServerConfig(&m_Settings, 5, 3),
	m_pTempConfigFile(NULL),
	m_pTrayIcon(NULL),
	m_pTrayIconMenu(NULL),
	m_alreadyHidden(false),
	m_SetupWizard(NULL),
	m_ElevateProcess(false),
	m_SuppressElevateWarning(false)
{
	setupUi(this);

	createMenuBar();
	loadSettings();
	initConnections();

	m_pUpdateIcon->hide();
	m_pUpdateLabel->hide();
	m_versionChecker.setApp(appPath(appConfig.synergycName()));
	m_pLabelScreenName->setText(getScreenName());
	m_pLabelIpAddresses->setText(getIPAddresses());

	m_SetupWizard = new SetupWizard(*this, false);

#if defined(Q_OS_WIN)
	// ipc must always be enabled, so that we can disable command when switching to desktop mode.
	connect(&m_IpcClient, SIGNAL(readLogLine(const QString&)), this, SLOT(appendLogRaw(const QString&)));
	connect(&m_IpcClient, SIGNAL(errorMessage(const QString&)), this, SLOT(appendLogError(const QString&)));
	connect(&m_IpcClient, SIGNAL(infoMessage(const QString&)), this, SLOT(appendLogNote(const QString&)));
	m_IpcClient.connectToHost();
#else
	// elevate checkbox is only useful on ms windows.
	m_pElevateCheckBox->hide();
#endif

    // change default size based on os
#if defined(Q_OS_MAC)
	resize(720, 550);
	setMinimumSize(size());
#elif defined(Q_OS_LINUX)
	resize(700, 530);
	setMinimumSize(size());
#endif
}

MainWindow::~MainWindow()
{
	if (appConfig().processMode() == Desktop)
	{
		stopDesktop();
	}

	saveSettings();
	delete m_SetupWizard;
}

void MainWindow::start(bool firstRun)
{
	onModeChanged(!firstRun && appConfig().autoConnect(), false);

	createTrayIcon();

	// always show. auto-hide only happens when we have a connection.
	showNormal();

	m_versionChecker.checkLatest();
}

void MainWindow::onModeChanged(bool startDesktop, bool applyService)
{
	refreshApplyButton();

	if (appConfig().processMode() == Service)
	{
		// form is always enabled in service mode.
		setFormEnabled(true);

		// ensure that the apply button actually does something, since desktop
		// mode screws around with connecting/disconnecting the action.
		disconnect(m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStartSynergy, SLOT(trigger()));
		connect(m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStartSynergy, SLOT(trigger()));

		if (applyService)
		{
			stopDesktop();
			startSynergy();
		}
	}
	else if ((appConfig().processMode() == Desktop) && startDesktop)
	{
		stopService();
		startSynergy();
	}

	m_pElevateCheckBox->setEnabled(appConfig().processMode() == Service);
}

void MainWindow::refreshApplyButton()
{
	m_pButtonApply->setEnabled(appConfig().processMode() == Service);
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

	connect(m_pTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));

	setIcon(synergyDisconnected);

	m_pTrayIcon->show();
}

void MainWindow::createMenuBar()
{
	QMenuBar* menubar = new QMenuBar(this);
	QMenu* pMenuFile = new QMenu(tr("&File"), menubar);
	QMenu* pMenuEdit = new QMenu(tr("&Edit"), menubar);
	QMenu* pMenuWindow = new QMenu(tr("&Window"), menubar);
	QMenu* pMenuHelp = new QMenu(tr("&Help"), menubar);

	menubar->addAction(pMenuFile->menuAction());
	menubar->addAction(pMenuEdit->menuAction());
#if !defined(Q_OS_MAC)
	menubar->addAction(pMenuWindow->menuAction());
#endif
	menubar->addAction(pMenuHelp->menuAction());

	pMenuFile->addAction(m_pActionStartSynergy);
	pMenuFile->addAction(m_pActionStopSynergy);
	pMenuFile->addSeparator();
	pMenuFile->addAction(m_pActionWizard);
	pMenuFile->addAction(m_pActionSave);
	pMenuFile->addSeparator();
	pMenuFile->addAction(m_pActionQuit);
	pMenuEdit->addAction(m_pActionSettings);
	pMenuWindow->addAction(m_pActionMinimize);
	pMenuWindow->addAction(m_pActionRestore);
	pMenuHelp->addAction(m_pActionAbout);

	setMenuBar(menubar);
}

void MainWindow::loadSettings()
{
	// the next two must come BEFORE loading groupServerChecked and groupClientChecked or
	// disabling and/or enabling the right widgets won't automatically work
        m_pRadioExternalConfig->setChecked(settings().value("useExternalConfig", false).toBool());
        m_pRadioInternalConfig->setChecked(settings().value("useInternalConfig", true).toBool());

	m_pGroupServer->setChecked(settings().value("groupServerChecked", false).toBool());
	m_pLineEditConfigFile->setText(settings().value("configFile", QDir::homePath() + "/" + synergyConfigName).toString());
	m_pGroupClient->setChecked(settings().value("groupClientChecked", true).toBool());
	m_pLineEditHostname->setText(settings().value("serverHostname").toString());

	m_SuppressElevateWarning = true;
	m_pElevateCheckBox->setChecked(settings().value("elevateChecked", false).toBool());
	m_SuppressElevateWarning = false;
}

void MainWindow::initConnections()
{
	connect(m_pActionMinimize, SIGNAL(triggered()), this, SLOT(hide()));
	connect(m_pActionRestore, SIGNAL(triggered()), this, SLOT(showNormal()));
	connect(m_pActionStartSynergy, SIGNAL(triggered()), this, SLOT(startSynergy()));
	connect(m_pActionStopSynergy, SIGNAL(triggered()), this, SLOT(stopSynergy()));
	connect(m_pActionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(&m_versionChecker, SIGNAL(updateFound(const QString&)), this, SLOT(updateFound(const QString&)));
}

void MainWindow::saveSettings()
{
	// program settings
	settings().setValue("groupServerChecked", m_pGroupServer->isChecked());
        settings().setValue("useExternalConfig", m_pRadioExternalConfig->isChecked());
	settings().setValue("configFile", m_pLineEditConfigFile->text());
        settings().setValue("useInternalConfig", m_pRadioInternalConfig->isChecked());
	settings().setValue("groupClientChecked", m_pGroupClient->isChecked());
	settings().setValue("serverHostname", m_pLineEditHostname->text());

	settings().sync();
}

void MainWindow::setIcon(qSynergyState state)
{
	QIcon icon;
	icon.addFile(synergyIconFiles[state]);

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
	m_pUpdateIcon->show();
	m_pUpdateLabel->show();
	m_pUpdateLabel->setText(
		tr("<p>Version %1 is now available, <a href=\"%2\">visit website</a>.</p>")
		.arg(version).arg("http://synergy-foss.org"));
}

void MainWindow::appendLogNote(const QString& text)
{
	appendLogRaw("NOTE: " + text);
}

void MainWindow::appendLogError(const QString& text)
{
	appendLogRaw("ERROR: " + text);
}

void MainWindow::appendLogRaw(const QString& text)
{
	foreach(QString line, text.split(QRegExp("\r|\n|\r\n"))) {
		if (!line.isEmpty()) {
			m_pLogOutput->append(line);
			updateStateFromLogLine(line);
		}
	}
}

void MainWindow::updateStateFromLogLine(const QString &line)
{
	// TODO: implement ipc connection state messages to replace this hack.
	if (line.contains("started server") ||
		line.contains("connected to server"))
	{
		setSynergyState(synergyConnected);

		// only hide once after each new connection.
		if (!m_alreadyHidden && appConfig().autoHide())
		{
			hide();
			m_alreadyHidden = true;
		}
	}
}

void MainWindow::clearLog()
{
    m_pLogOutput->clear();
}

void MainWindow::startSynergy()
{
	// TODO: refactor this out into 2 methods.
	bool desktopMode = appConfig().processMode() == Desktop;
	bool serviceMode = appConfig().processMode() == Service;

	if (desktopMode)
	{
		stopSynergy();
	}

	setSynergyState(synergyConnecting);

	QString app;
	QStringList args;

	args << "-f" << "--no-tray" << "--debug" << appConfig().logLevelText();

	if (!appConfig().screenName().isEmpty())
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
		args << "--stop-on-desk-switch";
#endif
	}

	if ((synergyType() == synergyClient && !clientArgs(args, app))
		|| (synergyType() == synergyServer && !serverArgs(args, app)))
	{
		if (desktopMode)
		{
			stopSynergy();
			return;
		}
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

	appendLogNote("starting " + QString(synergyType() == synergyServer ? "server" : "client"));

	// show command if debug log level...
	if (appConfig().logLevel() >= 4) {
		appendLogNote(QString("command: %1 %2").arg(app, args.join(" ")));
	}

	appendLogNote("config file: " + configFilename());
	appendLogNote("log level: " + appConfig().logLevelText());

	if (appConfig().logToFile())
		appendLogNote("log file: " + appConfig().logFilename());

	if (desktopMode)
	{
		synergyProcess()->start(app, args);
		if (!synergyProcess()->waitForStarted())
		{
			stopSynergy();
			show();
			QMessageBox::warning(this, tr("Program can not be started"), QString(tr("The executable<br><br>%1<br><br>could not be successfully started, although it does exist. Please check if you have sufficient permissions to run this program.").arg(app)));
			return;
		}
	}

	if (serviceMode)
	{
		QString command(app + " " + args.join(" "));
		m_IpcClient.sendCommand(command, m_ElevateProcess);
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

	if (m_pLineEditHostname->text().isEmpty())
	{
		show();
		QMessageBox::warning(this, tr("Hostname is empty"),
							 tr("Please fill in a hostname for the synergy client to connect to."));
		return false;
	}

	if (appConfig().logToFile())
	{
		appConfig().persistLogDir();
		args << "--log" << appConfig().logFilename();
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
		m_pTempConfigFile = new QTemporaryFile();
		if (!m_pTempConfigFile->open())
		{
			QMessageBox::critical(this, tr("Cannot write configuration file"), tr("The temporary configuration file required to start synergy can not be written."));
			return false;
		}

		serverConfig().save(*m_pTempConfigFile);
		filename = m_pTempConfigFile->fileName();

		m_pTempConfigFile->close();
	}
	else
	{
		if (!QFile::exists(m_pLineEditConfigFile->text()))
		{
			if (QMessageBox::warning(this, tr("Configuration filename invalid"),
				tr("You have not filled in a valid configuration file for the synergy server. "
						"Do you want to browse for the configuration file now?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes
					|| !on_m_pButtonBrowseConfigFile_clicked())
				return false;
		}

		filename = m_pLineEditConfigFile->text();
	}
	return filename;
}

QString MainWindow::address()
{
	return (!appConfig().interface().isEmpty() ? appConfig().interface() : "") + ":" + QString::number(appConfig().port());
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

	if (appConfig().logToFile())
	{
		appConfig().persistLogDir();
		args << "--log" << appConfig().logFilename();
	}

	args << "-c" << configFilename() << "--address" << address();

	return true;
}

void MainWindow::stopSynergy()
{
	if (appConfig().processMode() == Service)
	{
		stopService();
	}
	else if (appConfig().processMode() == Desktop)
	{
		stopDesktop();
	}

	setSynergyState(synergyDisconnected);

	// HACK: deleting the object deletes the physical file, which is
	// bad, since it could be in use by the Windows service!
	//delete m_pTempConfigFile;
	m_pTempConfigFile = NULL;

	// reset so that new connects cause auto-hide.
	m_alreadyHidden = false;
}

void MainWindow::stopService()
{
	// send empty command to stop service from laucning anything.
	m_IpcClient.sendCommand("", m_ElevateProcess);
}

void MainWindow::stopDesktop()
{
	if (!synergyProcess()) {
		return;
	}

	appendLogNote("stopping synergy desktop process");

	if (synergyProcess()->isOpen())
		synergyProcess()->close();

	delete synergyProcess();
	setSynergyProcess(NULL);
}

void MainWindow::synergyFinished(int exitCode, QProcess::ExitStatus)
{
	// on Windows, we always seem to have an exit code != 0.
#if !defined(Q_OS_WIN)
	if (exitCode != 0)
	{
		QMessageBox::critical(this, tr("Synergy terminated with an error"), QString(tr("Synergy terminated unexpectedly with an exit code of %1.<br><br>Please see the log output for details.")).arg(exitCode));
		stopSynergy();
	}
#else
	Q_UNUSED(exitCode);
#endif

	setSynergyState(synergyDisconnected);
}

void MainWindow::setSynergyState(qSynergyState state)
{
	if (synergyState() == state)
		return;

	if (state == synergyConnected || state == synergyConnecting)
	{
		disconnect (m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStartSynergy, SLOT(trigger()));
		connect (m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStopSynergy, SLOT(trigger()));
		m_pButtonToggleStart->setText(tr("&Stop"));
	}
	else
	{
		disconnect (m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStopSynergy, SLOT(trigger()));
		connect (m_pButtonToggleStart, SIGNAL(clicked()), m_pActionStartSynergy, SLOT(trigger()));
		m_pButtonToggleStart->setText(tr("&Start"));
	}

	bool connected = state == synergyConnected;

	// only disable controls in desktop mode. in service mode, we can use the apply button.
	if (appConfig().processMode() == Desktop)
	{
		setFormEnabled(!connected);
	}

	m_pActionStartSynergy->setEnabled(!connected);
	m_pActionStopSynergy->setEnabled(connected);

	switch (state)
	{
	case synergyConnected: {
		QString mode(appConfig().processMode() == Service ? tr("service mode") : tr("desktop mode"));
		setStatus(tr("Synergy is running (%1).").arg(mode));
		break;
	}
	case synergyConnecting:
		setStatus(tr("Synergy is starting."));
		break;
	case synergyDisconnected:
		setStatus(tr("Synergy is not running."));
		break;
	}

	setIcon(state);

	m_SynergyState = state;
}

void MainWindow::setFormEnabled(bool enabled)
{
	m_pGroupClient->setEnabled(enabled);
	m_pGroupServer->setEnabled(enabled);
}

void MainWindow::setVisible(bool visible)
{
	m_pActionMinimize->setEnabled(visible);
	m_pActionRestore->setEnabled(!visible);
	QMainWindow::setVisible(visible);

#if MAC_OS_X_VERSION_10_7
	// dock hide only supported on lion :(
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	GetCurrentProcess(&psn);
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

			result += format.arg(address);
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

void MainWindow::on_m_pActionSettings_triggered()
{
	ProcessMode lastProcessMode = appConfig().processMode();

	SettingsDialog dlg(this, appConfig());
	dlg.exec();

	if (lastProcessMode != appConfig().processMode())
	{
		onModeChanged(true, true);
	}
}

void MainWindow::on_m_pButtonConfigureServer_clicked()
{
	ServerConfigDialog dlg(this, serverConfig(), appConfig().screenName());
	dlg.exec();
}

void MainWindow::on_m_pActionWizard_triggered()
{
	m_SetupWizard->show();
}

void MainWindow::on_m_pElevateCheckBox_toggled(bool checked)
{
	if (checked && !m_SuppressElevateWarning) {
		int r = QMessageBox::warning(
			this, tr("Elevate Synergy"),
			tr("Are you sure you want to elevate Synergy?\n\n"
			   "This allows Synergy to interact with elevated processes "
			   "and the UAC dialog, but can cause problems with non-elevated "
			   "processes. Elevate Synergy only if you really need to."),
			QMessageBox::Yes | QMessageBox::No);

		if (r != QMessageBox::Yes) {
			m_pElevateCheckBox->setChecked(false);
			return;
		}
	}

	m_ElevateProcess = checked;
	settings().setValue("elevateChecked", checked);
	settings().sync();
}

void MainWindow::on_m_pButtonApply_clicked()
{
	startSynergy();
}

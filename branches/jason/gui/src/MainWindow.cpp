#include "MainWindow.h"
#include "AboutDialog.h"
#include "ServerConfigDialog.h"
#include "SettingsDialog.h"
#include "LogDialog.h"

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

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
	":/res/icons/16x16/synergy-connected.png"
};

static const char* logLevelNames[] =
{
	"ERROR",
	"WARNING",
	"NOTE",
	"INFO",
	"DEBUG",
	"DEBUG1",
	"DEBUG2"
};


MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	MainWindowBase(),
	m_Settings(),
	m_AppConfig(&m_Settings),
	m_pSynergy(NULL),
	m_SynergyState(synergyDisconnected),
	m_ServerConfig(&m_Settings, 5, 3),
	m_pTempConfigFile(NULL),
	m_pLogDialog(new LogDialog(this, synergyProcess())),
	m_pLabelStatusBar(NULL),
	m_pTrayIcon(NULL),
	m_pTrayIconMenu(NULL)
{
	setupUi(this);

	createTrayIcon();
	createMenuBar();
	createStatusBar();
	loadSettings();
	initConnections();

	if (appConfig().autoConnect())
		startSynergy();
}

MainWindow::~MainWindow()
{
	stopSynergy();
	saveSettings();
}

void MainWindow::createTrayIcon()
{
#if !defined(Q_OS_MAC)
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

	setIcon(synergyDisconnected);

	m_pTrayIcon->show();
#else
	setIcon(synergyDisconnected);
#endif
}

void MainWindow::createMenuBar()
{
	QMenuBar* menubar = new QMenuBar(this);
	QMenu* pMenuFile = new QMenu(tr("&File"), menubar);
	QMenu* pMenuEdit = new QMenu(tr("&Edit"), menubar);
	QMenu* pMenuView = new QMenu(tr("&View"), menubar);
	QMenu* pMenuWindow = new QMenu(tr("&Window"), menubar);
	QMenu* pMenuHelp = new QMenu(tr("&Help"), menubar);

	menubar->addAction(pMenuFile->menuAction());
	menubar->addAction(pMenuEdit->menuAction());
	menubar->addAction(pMenuView->menuAction());
#if !defined(Q_OS_MAC)
	menubar->addAction(pMenuWindow->menuAction());
#endif
	menubar->addAction(pMenuHelp->menuAction());

	pMenuFile->addAction(m_pActionStartSynergy);
	pMenuFile->addAction(m_pActionStopSynergy);
	pMenuFile->addSeparator();
	pMenuFile->addAction(m_pActionSave);
	pMenuFile->addSeparator();
	pMenuFile->addAction(m_pActionQuit);
	pMenuEdit->addAction(m_pActionSettings);
	pMenuView->addAction(m_pActionLogOutput);
	pMenuWindow->addAction(m_pActionMinimize);
	pMenuWindow->addAction(m_pActionRestore);
	pMenuHelp->addAction(m_pActionAbout);

	setMenuBar(menubar);
}

void MainWindow::createStatusBar()
{
	m_pLabelStatusBar = new QLabel(tr("Synergy is not running."));
	statusBar()->addPermanentWidget(m_pLabelStatusBar);
}

void MainWindow::loadSettings()
{
	// gui
	QRect rect = settings().value("windowGeometry", geometry()).toRect();
	move(rect.x(), rect.y());
	resize(rect.width(), rect.height());

#if !defined(Q_OS_MAC)
	setVisible(settings().value("windowVisible", true).toBool());
#else
	setVisible(true);
#endif

	// program settings

	// the next two must come BEFORE loading groupServerChecked and groupClientChecked or
	// disabling and/or enabling the right widgets won't automatically work
	m_pRadioExternalConfig->setChecked(settings().value("externalConfig", false).toBool());
	m_pRadioInternalConfig->setChecked(settings().value("internalConfig", true).toBool());

	m_pGroupServer->setChecked(settings().value("groupServerChecked", false).toBool());
	m_pLineEditConfigFile->setText(settings().value("configFile", QDir::homePath() + "/" + synergyConfigName).toString());
	m_pGroupClient->setChecked(settings().value("groupClientChecked", true).toBool());
	m_pLineEditHostname->setText(settings().value("serverHostname").toString());
}

void MainWindow::initConnections()
{
	connect(m_pActionMinimize, SIGNAL(triggered()), this, SLOT(hide()));
	connect(m_pActionRestore, SIGNAL(triggered()), this, SLOT(showNormal()));
	connect(m_pActionStartSynergy, SIGNAL(triggered()), this, SLOT(startSynergy()));
	connect(m_pActionStopSynergy, SIGNAL(triggered()), this, SLOT(stopSynergy()));
	connect(m_pActionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));

	if (m_pTrayIcon)
		connect(m_pTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}

void MainWindow::saveSettings()
{
	// gui
	settings().setValue("windowGeometry", geometry());

#if !defined(Q_OS_MAC)
	settings().setValue("windowVisible", isVisible());
#endif
	
	// program settings
	settings().setValue("groupServerChecked", m_pGroupServer->isChecked());
	settings().setValue("externalConfig", m_pRadioExternalConfig->isChecked());
	settings().setValue("configFile", m_pLineEditConfigFile->text());
	settings().setValue("internalConfig", m_pRadioInternalConfig->isChecked());
	settings().setValue("groupClientChecked", m_pGroupClient->isChecked());
	settings().setValue("serverHostname", m_pLineEditHostname->text());

	settings().sync();
}

void MainWindow::setIcon(qSynergyState state)
{
#if defined(Q_OS_WIN)
	setVisible(state == synergyDisconnected);
	m_pTrayIcon->setVisible(state == synergyDisconnected);
#endif

	QIcon icon;
	icon.addFile(synergyIconFiles[state]);

	if (m_pTrayIcon)
		m_pTrayIcon->setIcon(icon);

	setWindowIcon(icon);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::DoubleClick)
		setVisible(!isVisible());
}

void MainWindow::startSynergy()
{
	stopSynergy();

	QString app;
	QStringList args;

	args << "-f" << "--debug" << logLevelNames[appConfig().logLevel()];

	if (!appConfig().screenName().isEmpty())
		args << "--name" << appConfig().screenName();

	setSynergyProcess(new QProcess(this));

	if ((synergyType() == synergyClient && !clientArgs(args, app))
			|| synergyType() == synergyServer && !serverArgs(args, app))
	{
		stopSynergy();
		return;
	}

	connect(synergyProcess(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(synergyFinished(int, QProcess::ExitStatus)));
	connect(synergyProcess(), SIGNAL(readyReadStandardOutput()), m_pLogDialog, SLOT(readSynergyOutput()));
	connect(synergyProcess(), SIGNAL(readyReadStandardError()), m_pLogDialog, SLOT(readSynergyOutput()));

	m_pLogDialog->append(tr("\n\nRunning synergy: %1 %2\n\n").arg(app).arg(args.join(" ")));

	synergyProcess()->start(app, args);
	if (!synergyProcess()->waitForStarted())
	{
		stopSynergy();
		QMessageBox::warning(this, tr("Program can not be started"), QString(tr("The executable<br><br>%1<br><br>could not be successfully started, although it does exist. Please check if you have sufficient permissions to run this program.").arg(app)));
		return;
	}

	setSynergyState(synergyConnected);
}

bool MainWindow::clientArgs(QStringList& args, QString& app)
{
	app = appConfig().synergyc();

	if (!QFile::exists(app))
	{
		if (QMessageBox::warning(this, tr("Synergy client not found"), tr("The executable for the synergy client does not exist. Do you want to browse for the synergy client now?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return false;

		app = SettingsDialog::browseForSynergyc(this, appConfig().synergyProgramDir(), appConfig().synergycName());

		if (app.isEmpty())
			return false;

		appConfig().setSynergyc(app);
	}

	if (m_pLineEditHostname->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Hostname is empty"), tr("Please fill in a hostname for the synergy client to connect to."));
		return false;
	}

	args << m_pLineEditHostname->text() + ":" + QString::number(appConfig().port());

	return true;
}

bool MainWindow::serverArgs(QStringList& args, QString& app)
{
	app = appConfig().synergys();

	if (!QFile::exists(app))
	{
		if (QMessageBox::warning(this, tr("Synergy server not found"), tr("The executable for the synergy server does not exist. Do you want to browse for the synergy server now?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return false;

		app = SettingsDialog::browseForSynergys(this, appConfig().synergyProgramDir(), appConfig().synergysName());

		if (app.isEmpty())
			return false;

		appConfig().setSynergys(app);
	}

	if (m_pRadioInternalConfig->isChecked())
	{
		m_pTempConfigFile = new QTemporaryFile();
		if (!m_pTempConfigFile->open())
		{
			QMessageBox::critical(this, tr("Cannot write configuration file"), tr("The temporary configuration file required to start synergy can not be written."));
			return false;
		}

		serverConfig().save(*m_pTempConfigFile);
		args << "-c" << m_pTempConfigFile->fileName();

		m_pTempConfigFile->close();
		// the file will be removed from disk when the object is deleted; this happens in stopSynergy()
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

		args << "-c" << m_pLineEditConfigFile->text();
	}

	args << "--address" << (!appConfig().interface().isEmpty() ? appConfig().interface() : "") + ":" + QString::number(appConfig().port());

	return true;
}

void MainWindow::stopSynergy()
{
	if (synergyProcess())
	{
		if (synergyProcess()->isOpen())
			synergyProcess()->close();
		delete synergyProcess();
		setSynergyProcess(NULL);

		setSynergyState(synergyDisconnected);
	}

	delete m_pTempConfigFile;
	m_pTempConfigFile = NULL;
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

	// do not call stopSynergy() in case of clean synergy shutdown, because this must have (well, should have...)
	// come from our own delete synergyProcess() in stopSynergy(), so we would do a double-delete...
}

void MainWindow::setSynergyState(qSynergyState state)
{
	if (synergyState() == state)
		return;

	if (state == synergyConnected)
	{
		disconnect (m_pButtonStart, SIGNAL(clicked()), m_pActionStartSynergy, SLOT(trigger()));
		connect (m_pButtonStart, SIGNAL(clicked()), m_pActionStopSynergy, SLOT(trigger()));
		m_pButtonStart->setText(tr("&Stop"));
	}
	else
	{
		disconnect (m_pButtonStart, SIGNAL(clicked()), m_pActionStopSynergy, SLOT(trigger()));
		connect (m_pButtonStart, SIGNAL(clicked()), m_pActionStartSynergy, SLOT(trigger()));
		m_pButtonStart->setText(tr("&Start"));
	}

	m_pGroupClient->setEnabled(state == synergyDisconnected);
	m_pGroupServer->setEnabled(state == synergyDisconnected);
	m_pActionStartSynergy->setEnabled(state == synergyDisconnected);
	m_pActionStopSynergy->setEnabled(state == synergyConnected);
	m_pLabelStatusBar->setText(state == synergyConnected ? QString(tr("Synergy %1 is running.")).arg(synergyType() == synergyServer ? tr("server") : tr("client")) : tr("Synergy is not running."));
	setIcon(state);
	m_SynergyState = state;
}

void MainWindow::setVisible(bool visible)
{
	m_pActionMinimize->setEnabled(visible);
	m_pActionRestore->setEnabled(!visible);
	QMainWindow::setVisible(visible);
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
	AboutDialog dlg(this, appConfig().synergyc());
	dlg.exec();
}

void MainWindow::on_m_pActionSettings_triggered()
{
	SettingsDialog dlg(this, appConfig());
	dlg.exec();
}

void MainWindow::on_m_pActionLogOutput_triggered()
{
	Q_ASSERT(m_pLogDialog);

	m_pLogDialog->show();
	m_pLogDialog->raise();
	m_pLogDialog->activateWindow();
}

void MainWindow::on_m_pButtonConfigureServer_clicked()
{
	ServerConfigDialog dlg(this, serverConfig(), appConfig().screenName());
	dlg.exec();
}


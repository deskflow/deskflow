#include "WindowsServices.h"
#include "AppConfig.h"
#include "MainWindow.h"
#include "LogDialog.h"

#include <QWidget>
#include <QProcess>
#include <QMessageBox>
#include <QPushButton>

WindowsServices::WindowsServices(QWidget* parent, AppConfig& appConfig) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::WindowsServicesBase(),
	m_appConfig(appConfig),
	m_log(new LogDialog(this, process()))
{
	setupUi(this);
}

void WindowsServices::runProc(const QString& app, const QStringList& args, QPushButton* button)
{
	// disable until we know we've finished
	button->setEnabled(false);

	// clear contents so user doesn't get confused by previous messages
	m_log->clear();

	// deleted at end of function
	QProcess proc(this);
	m_process = &proc;

	// send output to log window
	connect(m_process, SIGNAL(readyReadStandardOutput()), m_log, SLOT(readSynergyOutput()));
	connect(m_process, SIGNAL(readyReadStandardError()), m_log, SLOT(readSynergyOutput()));

	m_process->start(app, args);
	m_log->show();

	// service management should be instant
	m_process->waitForFinished();

	if (m_process->exitCode() == 0)
	{
		QMessageBox::information(m_log, "Service manager", "Completed successfully.");
	}
	else
	{
		QMessageBox::critical(
			m_log, "Service manager error",
			QString("Unable to install or uninstall service. Error code: " +
					QString::number(m_process->exitCode())));
	}

	disconnect(m_process, SIGNAL(readyReadStandardOutput()), m_log, SLOT(readSynergyOutput()));
	disconnect(m_process, SIGNAL(readyReadStandardError()), m_log, SLOT(readSynergyOutput()));

	button->setEnabled(true);
}

void WindowsServices::on_m_pInstallServer_clicked()
{
	QString app = mainWindow()->appPath(
		appConfig().synergysName(), appConfig().synergys());

	QStringList args;
	args <<
		"--service" << "install" <<
		"--relaunch" <<
		"--debug" << appConfig().logLevelText() <<
		"-c" << mainWindow()->configFilename() <<
		"--address" << mainWindow()->address();

	if (appConfig().logToFile())
	{
		appConfig().persistLogDir();
		args << "--log" << appConfig().logFilename();
	}

	runProc(app, args, m_pInstallServer);
}

void WindowsServices::on_m_pUninstallServer_clicked()
{
	QString app = mainWindow()->appPath(
		appConfig().synergysName(), appConfig().synergys());

	QStringList args;
	args << "--service" << "uninstall";
	runProc(app, args, m_pInstallServer);
}

void WindowsServices::on_m_pInstallClient_clicked()
{
	if (mainWindow()->hostname().isEmpty())
	{
		QMessageBox::critical(
			this, "Service manager error", "Hostname was not specified on main screen.");
		return;
	}

	QString app = mainWindow()->appPath(
		appConfig().synergycName(), appConfig().synergyc());

	QStringList args;
	args <<
		"--service" << "install" <<
		"--relaunch" <<
		"--debug" << appConfig().logLevelText();

	if (appConfig().logToFile())
	{
		appConfig().persistLogDir();
		args << "--log" << appConfig().logFilename();
	}

	// hostname must come last to be a valid arg
	args << mainWindow()->hostname();

	runProc(app, args, m_pInstallServer);
}

void WindowsServices::on_m_pUninstallClient_clicked()
{
	QString app = mainWindow()->appPath(
		appConfig().synergycName(), appConfig().synergyc());

	QStringList args;
	args << "--service" << "uninstall";
	runProc(app, args, m_pInstallServer);
}

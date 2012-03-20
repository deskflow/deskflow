/*
 * synergy -- mouse and keyboard sharing utility
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

#include "WindowsServices.h"
#include "AppConfig.h"
#include "MainWindow.h"

#include <QWidget>
#include <QProcess>
#include <QMessageBox>
#include <QPushButton>

#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

WindowsServices::WindowsServices(MainWindow* mainWindow, AppConfig& appConfig) :
	QDialog(dynamic_cast<QWidget*>(mainWindow), Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::WindowsServicesBase(),
	m_mainWindow(mainWindow),
	m_appConfig(appConfig)
{
	setupUi(this);

	connect(m_pInstallServer, SIGNAL(clicked()), this, SLOT(installServer()));
	connect(m_pUninstallServer, SIGNAL(clicked()), this, SLOT(uninstallServer()));
	connect(m_pInstallClient, SIGNAL(clicked()), this, SLOT(installClient()));
	connect(m_pUninstallClient, SIGNAL(clicked()), this, SLOT(uninstallClient()));
}

bool WindowsServices::runProc(const QString& app, const QStringList& args, QPushButton* button)
{
	// disable until we know we've finished
	button->setEnabled(false);

	// separate from previous messages.
	m_mainWindow->appendLog("");

	// use ShellExecuteEx to run with elevation
	SHELLEXECUTEINFO execInfo = {0};
	execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	execInfo.lpVerb = L"runas"; // _T macro not available.
	execInfo.lpFile = reinterpret_cast<const wchar_t *>(app.utf16());
	execInfo.lpParameters = reinterpret_cast<const wchar_t *>(args.join(" ").utf16());
	execInfo.nShow = SW_HIDE;

	BOOL success = ShellExecuteEx(&execInfo);

	if (success)
	{
		m_mainWindow->appendLog(
			"service install/uninstall completed successfully.");
	}
	else
	{
		m_mainWindow->appendLog(
			"ERROR: failed to install/uninstall service.");
	}

	button->setEnabled(true);

	return success;
}

void WindowsServices::installServer()
{
	QString app = mainWindow()->appPath(appConfig().synergysName());

	QStringList args;
	args <<
		"--service" << "install" <<
		"--relaunch" <<
		"--no-tray" <<
		"--debug" << appConfig().logLevelText() <<
		"-c" << mainWindow()->configFilename() <<
		"--address" << mainWindow()->address();

	if (appConfig().logToFile())
	{
		appConfig().persistLogDir();
		args << "--log" << appConfig().logFilename();
	}

	if (runProc(app, args, m_pInstallServer))
	{
		appConfig().setServerService(true);
		appConfig().saveSettings();
	}
}

void WindowsServices::uninstallServer()
{
	QString app = mainWindow()->appPath(appConfig().synergysName());

	QStringList args;
	args << "--service" << "uninstall";

	if (runProc(app, args, m_pInstallServer))
	{
		appConfig().setServerService(false);
		appConfig().saveSettings();
	}
}

void WindowsServices::installClient()
{
	if (mainWindow()->hostname().isEmpty())
	{
		QMessageBox::critical(
			this, "Service manager error", "Hostname was not specified on main screen.");
		return;
	}

	QString app = mainWindow()->appPath(appConfig().synergycName());

	QStringList args;
	args <<
		"--service" << "install" <<
		"--relaunch" <<
		"--no-tray" <<
		"--debug" << appConfig().logLevelText();

	if (appConfig().logToFile())
	{
		appConfig().persistLogDir();
		args << "--log" << appConfig().logFilename();
	}

	// hostname must come last to be a valid arg
	args << mainWindow()->hostname();

	if (runProc(app, args, m_pInstallServer))
	{
		appConfig().setClientService(true);
		appConfig().saveSettings();
	}
}

void WindowsServices::uninstallClient()
{
	QString app = mainWindow()->appPath(appConfig().synergycName());

	QStringList args;
	args << "--service" << "uninstall";

	if (runProc(app, args, m_pInstallServer))
	{
		appConfig().setClientService(false);
		appConfig().saveSettings();
	}
}

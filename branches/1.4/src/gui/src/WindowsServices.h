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

#ifndef WINDOWSSERVICES_H
#define WINDOWSSERVICES_H

#include "ui_WindowsServicesBase.h"

class QWidget;
class QProcess;
class QPushButton;
class QProcess;

class AppConfig;
class MainWindow;

class WindowsServices : public QDialog, public Ui::WindowsServicesBase
{
	Q_OBJECT

	public:
		WindowsServices(MainWindow* parent, AppConfig& appConfig);

	protected:
		AppConfig &appConfig() const { return m_appConfig; }
		MainWindow* mainWindow() const { return (MainWindow*)parent(); }
		QProcess*& process() { return m_process; }
		void runProc(const QString& app, const QStringList& args, QPushButton* button);

	private:
		MainWindow* m_mainWindow;
		QString m_app;
		AppConfig &m_appConfig;
		QProcess* m_process;

	private slots:
		void on_m_pUninstallClient_clicked();
		void on_m_pInstallClient_clicked();
		void on_m_pUninstallServer_clicked();
		void on_m_pInstallServer_clicked();
};

#endif // WINDOWSSERVICES_H

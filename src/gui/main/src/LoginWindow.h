/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>

#include "ui_LoginWindowBase.h"

class MainWindow;
class SetupWizard;
class LoginAuth;
class AppConfig;

class LoginWindow : public QMainWindow, public Ui::LoginWindow
{
    Q_OBJECT
public:
	LoginWindow(MainWindow* mainWindow,
		SetupWizard* setupWizard,
		bool wizardShouldRun,
		QWidget *parent = 0);
    ~LoginWindow();

	void setLoginResult(int result) { m_LoginResult = result; }
	void setEditionType(int type) { m_EditionType = type; }
	void setError(QString error) { m_Error = error; }

protected:
    void changeEvent(QEvent *e);
	void closeEvent(QCloseEvent *event);

private slots:
	void on_m_pPushButtonCancel_clicked();
	void on_m_pPushButtonLogin_clicked();
	void showNext();

private:
	bool validEmailPassword();
private:
	MainWindow* m_pMainWindow;
	SetupWizard* m_pSetupWizard;
	bool m_WizardShouldRun;
	LoginAuth* m_pLoginAuth;
	int m_LoginResult;
	int m_EditionType;
	QString m_Error;
	AppConfig& m_AppConfig;

};

#endif // LOGINWINDOW_H

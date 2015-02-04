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

#include "LoginWindow.h"
#include "ui_LoginWindowBase.h"

#include "MainWindow.h"
#include "SetupWizard.h"
#include "LoginAuth.h"
#include "LoginResult.h"
#include "QUtility.h"

#include <QMessageBox>
#include <QCloseEvent>
#include <QThread>

LoginWindow::LoginWindow(
		MainWindow* mainWindow,
		SetupWizard* setupWizard,
		bool wizardShouldRun,
		QWidget *parent) :
	QMainWindow(parent),
	m_pMainWindow(mainWindow),
	m_pSetupWizard(setupWizard),
	m_WizardShouldRun(wizardShouldRun),
	m_pLoginAuth(NULL),
	m_LoginResult(Unknown),
	m_AppConfig(m_pMainWindow->appConfig())
{
	setupUi(this);

	m_pLineEditEmail->setText(m_AppConfig.userEmail());
}

LoginWindow::~LoginWindow()
{
	if (m_pLoginAuth != NULL) {
		delete m_pLoginAuth;
	}
}

void LoginWindow::showNext()
{
	if (m_LoginResult == ExceptionError) {
		QMessageBox::critical(
			this,
			tr("Error"),
			tr("Sorry, an error occured while trying to sign in. "
			"Please contact the help desk, and provide the "
			"following details.\n\n%1").arg(m_Error));
	}
	else if (m_LoginResult == InvalidEmailPassword) {
		QMessageBox::critical(
			this,
			tr("Error"),
			tr("Login failed, invalid email or password."));
	}
	else if (m_LoginResult == Error) {
		QMessageBox::critical(
			this,
			tr("Error"),
			tr("Login failed, an error occurred.\n\n%1").arg(m_Error));
	}
	else if (m_LoginResult == ServerResponseError) {
		QMessageBox::critical(
			this,
			"Error",
			tr("Login failed, an error occurred.\n\nServer response:\n\n%1")
			.arg(m_Error));
	}
	else {
		hide();
		if (m_WizardShouldRun) {
			m_pSetupWizard->show();
		}
		else {
			m_pMainWindow->setLoginResult(m_LoginResult);
			if (!m_pLineEditEmail->text().isEmpty()) {
				m_AppConfig.setUserEmail(m_pLineEditEmail->text());

				if (m_LoginResult != Unknown) {
					QString mac = getFirstMacAddress();
					QString hashSrc = m_pLineEditEmail->text() + mac;
					QString hashResult = hash(hashSrc);
					m_AppConfig.setUserToken(hashResult);
					m_AppConfig.setUserType(m_LoginResult);
				}
			}
			m_pMainWindow->show();
		}
	}

	delete m_pLoginAuth;
	m_pLoginAuth = NULL;
	m_LoginResult = Unknown;
	m_pPushButtonLogin->setText("Login");
	m_pPushButtonLogin->setDefault(true);
}

bool LoginWindow::validEmailPassword()
{
	if (m_pLineEditEmail->text().isEmpty() ||
		m_pLineEditPassword->text().isEmpty()) {
		QMessageBox::warning(
			this,
			"Warning",
			tr("Please fill in your email and password."));
		return false;
	}

	return true;
}

void LoginWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
		retranslateUi(this);
        break;
    default:
        break;
    }
}
void LoginWindow::closeEvent(QCloseEvent *event)
{
	event->accept();
	showNext();
}

void LoginWindow::on_m_pPushButtonLogin_clicked()
{
	if (validEmailPassword()) {
		if (m_pLoginAuth == NULL) {
			m_pLoginAuth = new LoginAuth();
			m_pLoginAuth->setLoginWindow(this);
		}

		m_pPushButtonLogin->setText("Logging...");

		QString email = m_pLineEditEmail->text();
		QString password = m_pLineEditPassword->text();
		m_pLoginAuth->setEmail(email);
		m_pLoginAuth->setPassword(password);

		QThread* thread = new QThread;
		connect(m_pLoginAuth, SIGNAL(finished()), this, SLOT(showNext()));
		connect(m_pLoginAuth, SIGNAL(finished()), thread, SLOT(quit()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

		m_pLoginAuth->moveToThread(thread);
		thread->start();

		QMetaObject::invokeMethod(m_pLoginAuth, "checkUserType", Qt::QueuedConnection);
	}
}

void LoginWindow::on_m_pPushButtonCancel_clicked()
{
	showNext();
}

#include "LoginDialog.h"

#include "MainWindow.h"
#include "SetupWizard.h"
#include "LoginAuth.h"
#include "LoginResult.h"
#include "EditionType.h"
#include "QUtility.h"

#include <QMessageBox>
#include <QCloseEvent>
#include <QThread>
#include <QDesktopServices>
#include <QUrl>

static const char registerUrl[] = "http://synergy-project.org";

LoginDialog::LoginDialog(
		MainWindow* mainWindow,
		SetupWizard* setupWizard,
		bool wizardShouldRun,
		QWidget *parent) :
	QDialog(parent),
	m_pMainWindow(mainWindow),
	m_pSetupWizard(setupWizard),
	m_WizardShouldRun(wizardShouldRun),
	m_pLoginAuth(NULL),
	m_LoginResult(Ok),
	m_EditionType(Unknown),
	m_AppConfig(m_pMainWindow->appConfig())
{
	setupUi(this);
	setFixedSize(size());
	m_pLineEditEmail->setText(m_AppConfig.userEmail());
}

LoginDialog::~LoginDialog()
{
	if (m_pLoginAuth != NULL) {
		delete m_pLoginAuth;
	}
}

void LoginDialog::showNext()
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
		QMessageBox::warning(
			this,
			tr("Warning"),
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
			m_pMainWindow->setEditionType(m_EditionType);
			if (!m_pLineEditEmail->text().isEmpty()) {
				m_AppConfig.setUserEmail(m_pLineEditEmail->text());

				if (m_EditionType != Unknown) {
					QString mac = getFirstMacAddress();
					QString hashSrc = m_pLineEditEmail->text() + mac;
					QString hashResult = hash(hashSrc);
					//m_AppConfig.setUserToken(hashResult);
					//m_AppConfig.setEditionType(m_EditionType);
				}
			}
			m_pMainWindow->show();
		}
	}

	delete m_pLoginAuth;
	m_pLoginAuth = NULL;
	m_LoginResult = Ok;
	m_EditionType = Unknown;
	m_pPushButtonLogin->setEnabled(true);
	m_pPushButtonLogin->setDefault(true);
	m_pLineEditEmail->setEnabled(true);
	m_pLineEditPassword->setEnabled(true);
}

bool LoginDialog::validEmailPassword()
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

void LoginDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		retranslateUi(this);
		break;
	default:
		break;
	}
}

void LoginDialog::closeEvent(QCloseEvent *event)
{
	event->accept();
	showNext();
}

void LoginDialog::on_m_pPushButtonLogin_clicked()
{
	if (validEmailPassword()) {
		if (m_pLoginAuth == NULL) {
			m_pLoginAuth = new LoginAuth();
			m_pLoginAuth->setLoginDialog(this);
		}

		m_pPushButtonLogin->setEnabled(false);
		m_pLineEditEmail->setEnabled(false);
		m_pLineEditPassword->setEnabled(false);

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

		QMetaObject::invokeMethod(m_pLoginAuth, "checkEditionType", Qt::QueuedConnection);
	}
}

void LoginDialog::keyPressEvent(QKeyEvent* e) {
	if(e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
	else {
		close();
	}
}

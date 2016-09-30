#include "ActivationDialog.h"
#include "ui_ActivationDialog.h"
#include "CancelActivationDialog.h"
#include "AppConfig.h"
#include "WebClient.h"
#include "EditionType.h"
#include "ActivationNotifier.h"
#include "MainWindow.h"
#include "QUtility.h"
#include "SubscriptionManager.h"

#include <QMessageBox>
#include <QThread>
#include <iostream>

ActivationDialog::ActivationDialog(QWidget* parent, AppConfig& appConfig) :
	QDialog(parent),
	ui(new Ui::ActivationDialog),
	m_appConfig (&appConfig)
{
	ui->setupUi(this);

	ui->m_pLineEditEmail->setText(appConfig.activateEmail());
	ui->m_pTextEditSerialKey->setText(appConfig.serialKey());

	if (!appConfig.serialKey().isEmpty()) {
		ui->m_pRadioButtonActivate->setAutoExclusive(false);
		ui->m_pRadioButtonSubscription->setAutoExclusive(false);
		ui->m_pRadioButtonActivate->setChecked(false);
		ui->m_pRadioButtonSubscription->setChecked(true);
		ui->m_pRadioButtonActivate->setAutoExclusive(true);
		ui->m_pRadioButtonSubscription->setAutoExclusive(true);
		ui->m_pTextEditSerialKey->setFocus();
		ui->m_pTextEditSerialKey->moveCursor(QTextCursor::End);
	} else {
		if (ui->m_pLineEditEmail->text().isEmpty()) {
			ui->m_pLineEditEmail->setFocus();
		} else {
			ui->m_pLineEditPassword->setFocus();
		}
	}
}

ActivationDialog::~ActivationDialog()
{
	delete ui;
}

void ActivationDialog::notifyActivation(QString identity)
{
	ActivationNotifier* notifier = new ActivationNotifier();
	notifier->setIdentity(identity);
	
	QThread* thread = new QThread();
	connect(notifier, SIGNAL(finished()), thread, SLOT(quit()));
	connect(notifier, SIGNAL(finished()), notifier, SLOT(deleteLater()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

	notifier->moveToThread(thread);
	thread->start();

	QMetaObject::invokeMethod(notifier, "notify", Qt::QueuedConnection);
}

void ActivationDialog::reject()
{
	CancelActivationDialog cancelActivationDialog(this);
	if (QDialog::Accepted == cancelActivationDialog.exec()) {
		notifyActivation("skip:unknown");
		QDialog::reject();
	}
}

void ActivationDialog::on_m_pRadioButtonSubscription_toggled(bool checked)
{
	if (checked) {
		ui->m_pLineEditEmail->setEnabled(false);
		ui->m_pLineEditPassword->setEnabled(false);
		ui->m_pTextEditSerialKey->setEnabled(true);
		ui->m_pTextEditSerialKey->setFocus();
	}
}

void ActivationDialog::on_m_pRadioButtonActivate_toggled(bool checked)
{
	if (checked) {
		ui->m_pLineEditEmail->setEnabled(true);
		ui->m_pLineEditPassword->setEnabled(true);
		ui->m_pTextEditSerialKey->setEnabled(false);
		if (ui->m_pLineEditEmail->text().isEmpty()) {
			ui->m_pLineEditEmail->setFocus();
		} else {
			ui->m_pLineEditPassword->setFocus();
		}
	}
}

void ActivationDialog::accept()
{
	QMessageBox message;
	QString error;
	int edition = Unregistered;

	try {
		if (ui->m_pRadioButtonActivate->isChecked()) {
			WebClient webClient;
			QString email = ui->m_pLineEditEmail->text();
			QString password = ui->m_pLineEditPassword->text();

			if (!webClient.setEmail (email, error)) {
				message.critical (this, "Invalid Email Address", tr("%1").arg(error));
				return;
			}
			else if (!webClient.setPassword (password, error)) {
				message.critical (this, "Invalid Password", tr("%1").arg(error));
				return;
			}
			else if (!webClient.getEdition (edition, error)) {
				message.critical (this, "Activation Error",
					tr("An error occurred while trying to activate Synergy. "
						"The Symless server returned the following error:\n\n%1").arg(error));
				return;
			}

			m_appConfig->setActivateEmail (email);
			m_appConfig->clearSerialKey();
			ui->m_pTextEditSerialKey->clear();
			notifyActivation ("login:" + m_appConfig->activateEmail());
		}
		else {
			QString serialKey = ui->m_pTextEditSerialKey->toPlainText();

			if (!m_appConfig->setSerialKey (serialKey, error)) {
				message.critical (this, "Invalid Serial Key", tr("%1").arg(error));
				return;
			}

			SubscriptionManager subscriptionManager (this, *m_appConfig, edition);
			if (!subscriptionManager.activateSerial (serialKey)) {
				return;
			}
			m_appConfig->setActivateEmail("");
			notifyActivation ("serial:" + m_appConfig->serialKey());
		}
	}
	catch (std::exception& e) {
		message.critical (this, "Unknown Error",
			tr("An error occurred while trying to activate Synergy. "
				"Please contact the helpdesk, and provide the "
				"following details.\n\n%1").arg(e.what()));
		return;
	}

	m_appConfig->setEdition(edition);
	m_appConfig->saveSettings();

	message.information  (this, "Activated!", 
						  tr("Thanks for activating %1!").arg 
							(getEditionName (edition)));
	MainWindow& mainWindow = dynamic_cast<MainWindow&>(*this->parent());
	mainWindow.setEdition(edition);
	mainWindow.updateLocalFingerprint();
	mainWindow.settings().sync();

	QDialog::accept();
}

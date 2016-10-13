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
#include "FailedLoginDialog.h"

#include <QMessageBox>
#include <QThread>
#include <iostream>

ActivationDialog::ActivationDialog(QWidget* parent, AppConfig& appConfig) :
	QDialog(parent),
	ui(new Ui::ActivationDialog),
	m_appConfig(&appConfig)
{
	ui->setupUi(this);
	ui->m_pTextEditSerialKey->setFocus();
	ui->m_pTextEditSerialKey->moveCursor(QTextCursor::End);
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
		m_appConfig->activationHasRun(true);
		m_appConfig->saveSettings();
		QDialog::reject();
	}
}

void ActivationDialog::accept()
{
	QMessageBox message;
	QString error;
	int edition = Unregistered;

	m_appConfig->activationHasRun(true);
	m_appConfig->saveSettings();

	try {
		QString serialKey = ui->m_pTextEditSerialKey->toPlainText();

		if (!m_appConfig->setSerialKey(serialKey, error)) {
			message.critical(this, "Invalid Serial Key", tr("%1").arg(error));
			return;
		}

		SubscriptionManager subscriptionManager(this, *m_appConfig, edition);
		if (!subscriptionManager.activateSerial(serialKey)) {
			return;
		}

		notifyActivation("serial:" + m_appConfig->serialKey());

	}
	catch (std::exception& e) {
		message.critical(this, "Unknown Error",
			tr("An error occurred while trying to activate Synergy. "
				"Please contact the helpdesk, and provide the "
				"following details.\n\n%1").arg(e.what()));
		return;
	}

	m_appConfig->setEdition(edition);
	m_appConfig->saveSettings();

	message.information(this, "Activated!",
				tr("Thanks for activating %1!").arg(getEditionName(edition)));
	QDialog::accept();
}

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

ActivationDialog::ActivationDialog(QWidget* parent, AppConfig& appConfig,
                                   SubscriptionManager& subscriptionManager) :
    QDialog(parent),
    ui(new Ui::ActivationDialog),
    m_appConfig(&appConfig),
    m_subscriptionManager (&subscriptionManager)
{
    ui->setupUi(this);
    refreshSerialKey();
}

void ActivationDialog::refreshSerialKey()
{
    ui->m_pTextEditSerialKey->setText(m_appConfig->serialKey());
    ui->m_pTextEditSerialKey->setFocus();
    ui->m_pTextEditSerialKey->moveCursor(QTextCursor::End);
}

ActivationDialog::~ActivationDialog()
{
    delete ui;
}

void ActivationDialog::reject()
{
    if (m_subscriptionManager->activeEdition() == kUnregistered) {
        CancelActivationDialog cancelActivationDialog(this);
        if (QDialog::Accepted == cancelActivationDialog.exec()) {
            m_subscriptionManager->skipActivation();
            m_appConfig->activationHasRun(true);
            m_appConfig->saveSettings();
        }
    }
    QDialog::reject();
}

void ActivationDialog::accept()
{
    QMessageBox message;
    m_appConfig->activationHasRun(true);
    m_appConfig->saveSettings();

	std::pair<bool, QString> result;
    try {
        QString serialKey = ui->m_pTextEditSerialKey->toPlainText();
        result = m_subscriptionManager->setSerialKey(serialKey);
    }
    catch (std::exception& e) {
        message.critical(this, "Unknown Error",
            tr("An error occurred while trying to activate Synergy. "
                "Please contact the helpdesk, and provide the "
                "following information:\n\n%1").arg(e.what()));
        refreshSerialKey();
        return;
    }

	if (!result.first) {
		message.critical(this, "Activation failed", 
						 tr("%1").arg(result.second));
		refreshSerialKey();
		return;
	}

    if (m_subscriptionManager->activeEdition() != kUnregistered) {
        message.information(this, "Activated!",
                    tr("Thanks for activating %1!").arg
                            (m_subscriptionManager->activeEditionName()));
    }

    QDialog::accept();
}

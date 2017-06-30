#include "ActivationDialog.h"
#include "ui_ActivationDialog.h"
#include "CancelActivationDialog.h"
#include "AppConfig.h"
#include "WebClient.h"
#include <shared/EditionType.h>
#include "ActivationNotifier.h"
#include "MainWindow.h"
#include "QUtility.h"
#include "LicenseManager.h"
#include "FailedLoginDialog.h"

#include <QMessageBox>
#include <QThread>
#include <iostream>

ActivationDialog::ActivationDialog (QWidget* parent, AppConfig& appConfig,
                                    LicenseManager& licenseManager)
    : QDialog (parent),
      ui (new Ui::ActivationDialog),
      m_appConfig (&appConfig),
      m_LicenseManager (&licenseManager) {
    ui->setupUi (this);
    refreshSerialKey ();
    time_t currentTime = ::time (0);
    if (!m_LicenseManager->serialKey ().isExpired (currentTime)) {
        ui->m_trialWidget->hide ();
    }
}

void
ActivationDialog::refreshSerialKey () {
    ui->m_pTextEditSerialKey->setText (m_appConfig->serialKey ());
    ui->m_pTextEditSerialKey->setFocus ();
    ui->m_pTextEditSerialKey->moveCursor (QTextCursor::End);
    ui->m_trialLabel->setText (tr ("<html><head/><body><p>Your trial has "
                                   "expired. <a href=\"https://symless.com/"
                                   "synergy/trial/thanks?id=%1\"><span "
                                   "style=\"text-decoration: underline; "
                                   "color:#0000ff;\">Buy now!</span></a>"
                                   "</p></body></html>")
                                   .arg (m_appConfig->serialKey ()));
}

ActivationDialog::~ActivationDialog () {
    delete ui;
}

void
ActivationDialog::reject () {
    if (m_LicenseManager->activeEdition () == kUnregistered) {
        CancelActivationDialog cancelActivationDialog (this);
        if (QDialog::Accepted == cancelActivationDialog.exec ()) {
            m_LicenseManager->skipActivation ();
            m_appConfig->activationHasRun (true);
            m_appConfig->saveSettings ();
        } else {
            return;
        }
    }
    QDialog::reject ();
}

void
ActivationDialog::accept () {
    QMessageBox message;
    m_appConfig->activationHasRun (true);
    m_appConfig->saveSettings ();

    std::pair<bool, QString> result;
    try {
        SerialKey serialKey (
            ui->m_pTextEditSerialKey->toPlainText ().trimmed ().toStdString ());
        result = m_LicenseManager->setSerialKey (serialKey);
    } catch (std::exception& e) {
        message.critical (
            this,
            "Unknown Error",
            tr ("An error occurred while trying to activate Synergy. "
                "Please contact the helpdesk, and provide the "
                "following information:\n\n%1")
                .arg (e.what ()));
        refreshSerialKey ();
        return;
    }

    if (!result.first) {
        message.critical (
            this, "Activation failed", tr ("%1").arg (result.second));
        refreshSerialKey ();
        return;
    }

    m_LicenseManager->notifyActivation ("serial:" + m_appConfig->serialKey ());
    Edition edition = m_LicenseManager->activeEdition ();
    time_t daysLeft = m_LicenseManager->serialKey ().daysLeft (::time (0));
    if (edition != kUnregistered) {
        QString thanksMessage =
            tr ("Thanks for trying %1! %5\n\n%2 day%3 of "
                "your trial remain%4")
                .arg (m_LicenseManager->getEditionName (edition))
                .arg (daysLeft)
                .arg ((daysLeft == 1) ? "" : "s")
                .arg ((daysLeft == 1) ? "s" : "");

        if (edition == kPro) {
            thanksMessage =
                thanksMessage.arg ("If you're using SSL, "
                                   "remember to activate all of your devices.");
        } else {
            thanksMessage = thanksMessage.arg ("");
        }

        if (m_LicenseManager->serialKey ().isTrial ()) {
            message.information (this, "Thanks!", thanksMessage);
        } else {
            message.information (
                this,
                "Activated!",
                tr ("Thanks for activating %1!")
                    .arg (m_LicenseManager->getEditionName (edition)));
        }
    }

    QDialog::accept ();
}

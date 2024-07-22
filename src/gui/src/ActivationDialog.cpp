/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Synergy Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ActivationDialog.h"
#include "AppConfig.h"
#include "CancelActivationDialog.h"
#include "LicenseManager.h"
#include "MainWindow.h"
#include "shared/EditionType.h"
#include "ui_ActivationDialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QThread>

ActivationDialog::ActivationDialog(
    QWidget *parent, AppConfig &appConfig, LicenseManager &licenseManager)
    : QDialog(parent),
      ui(new Ui::ActivationDialog),
      m_appConfig(&appConfig),
      m_LicenseManager(&licenseManager) {
  ui->setupUi(this);
  refreshSerialKey();
  time_t currentTime = ::time(0);
  if (!m_LicenseManager->serialKey().isExpired(currentTime)) {
    ui->m_trialWidget->hide();
  }
}

void ActivationDialog::refreshSerialKey() {
  ui->m_pTextEditSerialKey->setText(m_appConfig->serialKey());
  ui->m_pTextEditSerialKey->setFocus();
  ui->m_pTextEditSerialKey->moveCursor(QTextCursor::End);
  ui->m_trialLabel->setText(
      tr(m_LicenseManager->getLicenseNotice().toStdString().c_str()));
}

ActivationDialog::~ActivationDialog() { delete ui; }

void ActivationDialog::reject() {
  // don't show the cancel confirmation dialog if they've already registered,
  // since it's not revent to customers who are changing their serial key.
  if (m_LicenseManager->activeEdition() != kUnregistered) {
    QDialog::reject();
    return;
  }

  // the user is told that the 'No' button will exit the app.
  CancelActivationDialog cancelActivationDialog(this);
  if (cancelActivationDialog.exec() == QDialog::Rejected) {
    QApplication::exit();
  }
}

void ActivationDialog::accept() {
  QMessageBox message;
  m_appConfig->activationHasRun(true);

  try {
    SerialKey serialKey(
        ui->m_pTextEditSerialKey->toPlainText().trimmed().toStdString());
    m_LicenseManager->setSerialKey(serialKey);
  } catch (std::exception &e) {
    message.critical(
        this, "Activation failed",
        tr("An error occurred while trying to activate Synergy. "
           "<a href=\"https://symless.com/synergy/contact-support?source=gui\" "
           "style=\"text-decoration: none; color: #4285F4;\">"
           "Please contact the helpdesk</a>, and provide the following "
           "information:"
           "<br><br>%1")
            .arg(e.what()));
    refreshSerialKey();
    return;
  }

  m_LicenseManager->notifyActivation("serial:" + m_appConfig->serialKey());
  Edition edition = m_LicenseManager->activeEdition();
  time_t daysLeft = m_LicenseManager->serialKey().daysLeft(::time(0));
  if (edition != kUnregistered) {
    QString thanksMessage = tr("Thanks for trying %1! %5\n\n%2 day%3 of "
                               "your trial remain%4")
                                .arg(m_LicenseManager->getEditionName(edition))
                                .arg(daysLeft)
                                .arg((daysLeft == 1) ? "" : "s")
                                .arg((daysLeft == 1) ? "s" : "");

    if (m_appConfig->cryptoAvailable()) {
      m_appConfig->generateCertificate();
      thanksMessage =
          thanksMessage.arg("If you're using SSL, "
                            "remember to activate all of your devices.");
    } else {
      thanksMessage = thanksMessage.arg("");
    }

    if (m_LicenseManager->serialKey().isTrial()) {
      message.information(this, "Thanks!", thanksMessage);
    } else {
      message.information(
          this, "Activated!",
          tr("Thanks for activating %1!")
              .arg(m_LicenseManager->getEditionName(edition)));
    }
  }

  QDialog::accept();
}

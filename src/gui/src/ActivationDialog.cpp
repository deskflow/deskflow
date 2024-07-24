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
#include "MainWindow.h"
#include "gui/LicenseDisplay.h"
#include "license/ProductEdition.h"
#include "license/parse_serial_key.h"
#include "ui_ActivationDialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QThread>

using namespace synergy::license;

const char *const kContactUrl =
    "https://symless.com/synergy/contact?source=gui";
const char *const kLinkStyle = "color: #4285F4";

ActivationDialog::ActivationDialog(
    QWidget *parent, AppConfig &appConfig, LicenseDisplay &licenseDisplay)
    : QDialog(parent),
      ui(new Ui::ActivationDialog),
      m_appConfig(&appConfig),
      m_LicenseDisplay(licenseDisplay) {
  ui->setupUi(this);
  refreshSerialKey();
  time_t currentTime = ::time(0);
  if (!m_LicenseDisplay.license().isExpired(currentTime)) {
    ui->m_trialWidget->hide();
  }
}

void ActivationDialog::refreshSerialKey() {
  ui->m_pTextEditSerialKey->setText(m_appConfig->serialKey());
  ui->m_pTextEditSerialKey->setFocus();
  ui->m_pTextEditSerialKey->moveCursor(QTextCursor::End);
  if (m_LicenseDisplay.license().isTrial()) {
    ui->m_trialLabel->setText(
        tr(m_LicenseDisplay.noticeMessage().toStdString().c_str()));
  }
}

ActivationDialog::~ActivationDialog() { delete ui; }

void ActivationDialog::reject() {
  // don't show the cancel confirmation dialog if they've already registered,
  // since it's not revent to customers who are changing their serial key.
  if (m_LicenseDisplay.productEdition() != Edition::kUnregistered) {
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
  m_appConfig->setActivationHasRun(true);

  auto serialKeyString =
      ui->m_pTextEditSerialKey->toPlainText().trimmed().toStdString();

  SerialKey serialKey = parseSerialKey(serialKeyString);

  if (serialKey.isValid) {
    m_LicenseDisplay.setLicense(License(serialKey));
  } else {
    QMessageBox::critical(
        this, "Activation failed",
        tr("<p>There was a problem activating Synergy. "
           "Please "
           R"(<a href="%1" style="%2">contact us</a>)"
           ", and provide the following information:"
           "</p>"
           "%3")
            .arg(kContactUrl)
            .arg(kLinkStyle)
            .arg(m_LicenseDisplay.noticeMessage()));
    refreshSerialKey();
  }

  Edition edition = m_LicenseDisplay.productEdition();
  time_t daysLeft = m_LicenseDisplay.license().daysLeft(::time(0));
  if (edition != Edition::kUnregistered) {
    QString thanksMessage = tr("Thanks for trying %1! %5\n\n%2 day%3 of "
                               "your trial remain%4")
                                .arg(m_LicenseDisplay.productName())
                                .arg(daysLeft)
                                .arg((daysLeft == 1) ? "" : "s")
                                .arg((daysLeft == 1) ? "s" : "");

    if (m_appConfig->tlsAvailable()) {
      m_appConfig->generateCertificate();
      thanksMessage =
          thanksMessage.arg("If you're using SSL, "
                            "remember to activate all of your devices.");
    } else {
      thanksMessage = thanksMessage.arg("");
    }

    if (m_LicenseDisplay.license().isTrial()) {
      QMessageBox::information(this, "Thanks!", thanksMessage);
    } else {
      QMessageBox::information(
          this, "Activated!",
          tr("Thanks for activating %1!").arg(m_LicenseDisplay.productName()));
    }
  }

  QDialog::accept();
}

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
#include <qglobal.h>

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
  if (!m_LicenseDisplay.license().isExpired()) {
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

  auto safeParseSerialKey = [this, &serialKeyString]() {
    try {
      return parseSerialKey(serialKeyString);
    } catch (const SerialKeyParseError &e) {
      showActivationError(e.what());
      return SerialKey::invalid();
    }
  };

  const auto &serialKey = safeParseSerialKey();
  if (!serialKey.isValid) {
    qDebug() << "Invalid serial key";
    return;
  }

  if (License license(serialKey); !license.isExpired()) {
    m_LicenseDisplay.setLicense(license);
  } else {
    showActivationError(m_LicenseDisplay.noticeMessage());
    refreshSerialKey();
  }

  if (m_LicenseDisplay.license().isTrial()) {
    showTrialMessage();
  } else {
    QMessageBox::information(
        this, "Activated",
        QString("Thanks for activating %1.")
            .arg(m_LicenseDisplay.productName()));
  }

  QDialog::accept();
}

void ActivationDialog::showTrialMessage() {
  auto daysLeft = m_LicenseDisplay.license().daysLeft().count();

  QString tlsMessage;
  if (m_appConfig->tlsAvailable()) {
    m_appConfig->generateCertificate();
    tlsMessage = "Remember to activate all of your devices to use TLS.";
  }

  QString message =
      tr("<p>Thanks for trying %1. %2</p><p>%3 %4 of your trial %5.</p>")
          .arg(m_LicenseDisplay.productName())
          .arg(tlsMessage)
          .arg(daysLeft)
          .arg((daysLeft == 1) ? "day" : "days")
          .arg((daysLeft == 1) ? "remains" : "remain");

  QMessageBox::information(this, "Thanks", message);
}

void ActivationDialog::showActivationError(const QString &message) {
  QString fullMessage =
      QString("<p>There was a problem activating Synergy.</p>"
              R"(<p>Please <a href="%1" style="%2">contact us</a> )"
              "and provide the following information:</p>"
              "%3")
          .arg(kContactUrl)
          .arg(kLinkStyle)
          .arg(message);
  QMessageBox::critical(this, "Activation failed", fullMessage);
}
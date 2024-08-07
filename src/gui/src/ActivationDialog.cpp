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

#include "CancelActivationDialog.h"
#include "MainWindow.h"
#include "gui/config/AppConfig.h"
#include "gui/constants.h"
#include "gui/license/LicenseHandler.h"
#include "gui/license/license_notices.h"
#include "gui/styles.h"
#include "license/ProductEdition.h"
#include "license/parse_serial_key.h"
#include "ui_ActivationDialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QThread>

using namespace synergy::gui;
using namespace synergy::license;

ActivationDialog::ActivationDialog(
    QWidget *parent, AppConfig &appConfig, LicenseHandler &licenseHandler)
    : QDialog(parent),
      m_ui(new Ui::ActivationDialog),
      m_pAppConfig(&appConfig),
      m_licenseHandler(licenseHandler) {

  m_ui->setupUi(this);

  m_ui->m_pLabelNotice->setStyleSheet(kStyleNoticeLabel);

  refreshSerialKey();

  if (!m_licenseHandler.license().isExpired()) {
    m_ui->m_widgetNotice->hide();
  }

  const QString envSerialKey = ::getenv("SYNERGY_TEST_SERIAL_KEY");
  if (!envSerialKey.isEmpty()) {
    qDebug() << "using env test serial key:" << envSerialKey;
    m_ui->m_pTextEditSerialKey->setText(envSerialKey);
  }
}

void ActivationDialog::refreshSerialKey() {
  m_ui->m_pTextEditSerialKey->setText(m_pAppConfig->serialKey());
  m_ui->m_pTextEditSerialKey->setFocus();
  m_ui->m_pTextEditSerialKey->moveCursor(QTextCursor::End);

  const auto &license = m_licenseHandler.license();
  if (license.isTimeLimited()) {
    m_ui->m_pLabelNotice->setText(licenseNotice(license));
  }
}

ActivationDialog::~ActivationDialog() { delete m_ui; }

void ActivationDialog::reject() {
  // don't show the cancel confirmation dialog if they've already registered,
  // since it's not revent to customers who are changing their serial key.
  if (m_licenseHandler.productEdition() != Edition::kUnregistered) {
    QDialog::reject();
    return;
  }

  // the accept button should be labeled "Exit" on the cancel dialoig.
  CancelActivationDialog cancelActivationDialog(this);
  if (cancelActivationDialog.exec() == QDialog::Accepted) {
    QApplication::exit();
  }
}

void ActivationDialog::accept() {
  using Result = LicenseHandler::ChangeSerialKeyResult;
  auto serialKey = m_ui->m_pTextEditSerialKey->toPlainText();

  if (serialKey.isEmpty()) {
    QMessageBox::information(this, "Activation", "Please enter a serial key.");
    return;
  }

  try {
    const auto result = m_licenseHandler.changeSerialKey(serialKey);
    if (result != Result::kSuccess) {
      showResultDialog(result);
      return;
    }
  } catch (const SerialKeyParseError &e) {
    showErrorDialog(e.what());
    return;
  }

  showSuccessDialog();
  QDialog::accept();
}

void ActivationDialog::showResultDialog(
    LicenseHandler::ChangeSerialKeyResult result) {
  const QString title = "Activation result";

  switch (result) {
    using enum LicenseHandler::ChangeSerialKeyResult;

  case kUnchanged:
    QMessageBox::information(
        this, title,
        "Heads up, the serial key you entered was the same as last time.");
    QDialog::accept();
    break;

  case kInvalid:
    QMessageBox::critical(
        this, title,
        QString(
            "Invalid serial key. "
            R"(Please <a href="%1" style="color: %2">contact us</a> for help.)")
            .arg(kUrlContact)
            .arg(kColorSecondary));
    break;

  case kExpired:
    QMessageBox::warning(
        this, title,
        QString(
            "Sorry, that serial key has expired. "
            R"(Please <a href="%1" style="color: %1">renew</a> your license.)")
            .arg(kUrlPurchase)
            .arg(kColorSecondary));
    break;

  default:
    qFatal("unexpected change serial key result: %d", static_cast<int>(result));
  }
}

void ActivationDialog::showSuccessDialog() {
  const auto &license = m_licenseHandler.license();

  QString title = "Activation successful";
  QString message = tr("<p>Thanks for activating %1.</p>")
                        .arg(m_licenseHandler.productName());

  TlsUtility tls(*m_pAppConfig, license);
  if (tls.isAvailableAndEnabled()) {
    message +=
        "<p>To ensure that TLS encryption works correctly, "
        "please activate all of your computers with the same serial key.</p>";
  }

  if (license.isTimeLimited()) {
    auto daysLeft = license.daysLeft().count();
    if (license.isTrial()) {
      title = "Trial started";
      message += QString("Your trial will expire in %1 %2.")
                     .arg(daysLeft)
                     .arg((daysLeft == 1) ? "day" : "days");
    } else if (license.isSubscription()) {
      message += QString("Your license will expire in %1 %2.")
                     .arg(daysLeft)
                     .arg((daysLeft == 1) ? "day" : "days");
    }
  }

  QMessageBox::information(this, title, message);
}

void ActivationDialog::showErrorDialog(const QString &message) {
  QString fullMessage =
      QString("<p>There was a problem activating Synergy.</p>"
              R"(<p>Please <a href="%1" style="color: %2">contact us</a> )"
              "and provide the following information:</p>"
              "%3")
          .arg(kUrlContact)
          .arg(kColorSecondary)
          .arg(message);
  QMessageBox::critical(this, "Activation failed", fullMessage);
}

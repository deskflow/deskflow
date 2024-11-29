/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FingerprintDialog.h"
#include "ui_FingerprintDialog.h"

#include "net/SecureUtils.h"

#include <QFont>
#include <QPushButton>

FingerprintDialog::FingerprintDialog(
    QWidget *parent, const QList<deskflow::FingerprintData> &fingerprints, FingerprintDialogMode mode
)
    : QDialog(parent),
      ui{new Ui::FingerprintDialog}
{
  ui->setupUi(this);

  if (mode == FingerprintDialogMode::Local) {
    setWindowTitle(tr("Your Fingerprints"));
    ui->lblBody->setText(tr("These are the fingerprints for this computer"));
    ui->lblFooter->setVisible(false);
  } else {
    auto body = tr("Compare the fingerprints in this dialog to those on the %1.\n"
                   "Only accept this dialog if they match!");

    if (mode == FingerprintDialogMode::Server) {
      ui->lblBody->setText(tr("A new Client is connecting\n%1").arg(body.arg(tr("client"))));
    } else {
      ui->lblBody->setText(tr("You are connecting to a new server\n%1").arg(body.arg(tr("server"))));
    }

    setWindowTitle(tr("Security Question"));
    ui->lblFooter->setText(tr("<p>Do you want to trust this fingerprint for future "
                              "connections? If you don't, a connection cannot be made.</p>"));

    ui->buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
    connect(ui->buttonBox->button(QDialogButtonBox::Yes), &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->buttonBox->button(QDialogButtonBox::No), &QPushButton::clicked, this, &QDialog::reject);
  }

  for (const auto &fingerprint : fingerprints) {
    if (fingerprint.algorithm == "sha1") {
      ui->lblSHA1->setText(QString::fromStdString(deskflow::formatSSLFingerprint(fingerprint.data)));
    }

    if (fingerprint.algorithm == "sha256") {
      ui->lblSHA256->setText(QString::fromStdString(deskflow::formatSSLFingerprintColumns(fingerprint.data)));
      ui->lblSha256Art->setText(QString::fromStdString(deskflow::generateFingerprintArt(fingerprint.data)));
    }
  }

  QFont f = font();
  f.setFamilies({"Hack", "Liberation Mono", "Monospace", "Andale Mono"});
  f.setStyleHint(QFont::Monospace);
  ui->lblSha256Art->setFont(f);

  if (ui->lblSHA1->text().isEmpty()) {
    ui->sha1Frame->setVisible(false);
  }

  if (ui->lblSHA256->text().isEmpty()) {
    ui->sha256Frame->setVisible(false);
  }

  adjustSize();
}

FingerprintDialog::~FingerprintDialog()
{
  delete ui;
}

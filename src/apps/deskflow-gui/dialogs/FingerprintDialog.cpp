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

  if (mode == FingerprintDialogMode::Remote) {
    setWindowTitle(tr("Security Question"));
    ui->lblBody->setText(tr("<p>You are connecting to a server.</p>"
                            "<p>Compare the fingerprints below to the ones on your server. "
                            "If the two don't match exactly, then it's probably not the server "
                            "you're expecting (it could be a malicious user).\n</p>"));

    ui->lblFooter->setText(tr("<p>Do you want to trust this fingerprint for future "
                              "connections? If you don't, a connection cannot be made.</p>"));
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
    connect(ui->buttonBox->button(QDialogButtonBox::Yes), &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->buttonBox->button(QDialogButtonBox::No), &QPushButton::clicked, this, &QDialog::reject);
  } else {
    setWindowTitle(tr("Your Fingerprints"));
    ui->lblBody->setText(tr("These are the fingerprints for this computer"));
    ui->lblFooter->setVisible(false);
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

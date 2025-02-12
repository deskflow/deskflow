/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FingerprintDialog.h"

#include "widgets/FingerprintPreview.h"

#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

FingerprintDialog::FingerprintDialog(
    QWidget *parent, const QList<deskflow::FingerprintData> &fingerprints, FingerprintDialogMode mode
)
    : QDialog(parent),
      m_lblHeader{new QLabel(this)},
      m_lblFooter{new QLabel(this)},
      m_fingerprintPreview{new FingerprintPreview(this, fingerprints)},
      m_buttonBox{new QDialogButtonBox(this)}
{
  setWindowIcon(QIcon::fromTheme("fingerprint"));
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  m_lblHeader->setWordWrap(true);
  m_lblFooter->setWordWrap(true);
  m_lblFooter->setAlignment(Qt::AlignHCenter);

  auto layout = new QVBoxLayout();
  layout->addWidget(m_lblHeader);
  layout->addSpacerItem(new QSpacerItem(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
  layout->addWidget(m_fingerprintPreview, 0, Qt::AlignTop | Qt::AlignHCenter);
  layout->addWidget(m_lblFooter);
  layout->addWidget(m_buttonBox);
  setLayout(layout);

  if (mode == Local) {
    setWindowTitle(tr("Local Fingerprints"));
    m_lblHeader->setText(tr("Local computer's fingerprints"));
    m_lblFooter->setVisible(false);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Ok);
    connect(m_buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &QDialog::accept);
  } else {
    setWindowTitle(tr("Security Question"));
    auto body = tr("Compare the fingerprints in this dialog to those on the %1.\n"
                   "Only connect if they match!");

    if (mode == FingerprintDialogMode::Server) {
      m_lblHeader->setText(tr("A new client is connecting.\n%1").arg(body.arg(tr("client"))));
      m_lblFooter->setText(tr("\nDo you want connect to and trust the client?\n"));
    } else {
      m_lblHeader->setText(tr("You are connecting to a new server.\n%1").arg(body.arg(tr("server"))));
      m_lblFooter->setText(tr("\nDo you want connect to the server?\n"));
    }

    m_buttonBox->setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Yes | QDialogButtonBox::No);

    // Use help to request a dialog with the host prints
    // Help is used because its always to the furthest from the other buttons.
    m_buttonBox->button(QDialogButtonBox::Help)->setText(tr("View local fingerprints"));
    m_buttonBox->button(QDialogButtonBox::Help)->setIcon(QIcon::fromTheme("fingerprint"));
    m_buttonBox->button(QDialogButtonBox::Help)->setToolTip(tr("Show the local machines fingerprints"));
    connect(
        m_buttonBox->button(QDialogButtonBox::Help), &QPushButton::clicked, this,
        &FingerprintDialog::requestLocalPrintsDialog
    );

    m_buttonBox->button(QDialogButtonBox::No)->setFocus();
    connect(m_buttonBox->button(QDialogButtonBox::No), &QPushButton::clicked, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Yes), &QPushButton::clicked, this, &QDialog::accept);
  }
  adjustSize();
  setFixedSize(size());
}

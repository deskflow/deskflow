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
    QWidget *parent, const Fingerprint &localFingerprint, FingerprintDialogMode mode,
    const Fingerprint &remoteFingerprint
)
    : QDialog(parent),
      m_lblHeader{new QLabel(this)},
      m_lblFooter{new QLabel(this)},
      m_buttonBox{new QDialogButtonBox(QDialogButtonBox::Help, this)}
{
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  m_lblHeader->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  const bool localMode = mode == FingerprintDialogMode::Local;
  const bool isServer = mode == FingerprintDialogMode::Server;

  auto layout = new QVBoxLayout();
  layout->addWidget(m_lblHeader);
  layout->addSpacerItem(new QSpacerItem(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
  layout->addLayout(
      localMode ? makeLocalLayout(localFingerprint) : makeCompareLayout(localFingerprint, isServer, remoteFingerprint)
  );
  layout->addWidget(m_lblFooter);
  layout->addWidget(m_buttonBox);
  setLayout(layout);

  if (localMode) {
    setWindowTitle(tr("Local Fingerprints"));
    setWindowIcon(QIcon::fromTheme("fingerprint"));

    m_lblHeader->setText(tr("Local computer's fingerprint"));

    m_buttonBox->addButton(QDialogButtonBox::Ok);
    connect(m_buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &QDialog::accept);

  } else {
    setWindowIcon(QIcon::fromTheme("question"));
    m_lblHeader->setAlignment(Qt::AlignHCenter);
    m_lblFooter->setAlignment(Qt::AlignHCenter);

    auto body =
        tr("Compare the fingerprints in this dialog to those on the %1.\n"
           "Only connect if they match!");

    if (isServer) {
      setWindowTitle(tr("New client connecting"));
      m_lblHeader->setText(body.arg(tr("client")));
      m_lblFooter->setText(tr("\nTrust client and allow connection?\n"));
    } else {
      setWindowTitle(tr("Connecting to a new server"));
      m_lblHeader->setText(body.arg(tr("server")));
      m_lblFooter->setText(tr("\nDo you want to connect to the server?\n"));
    }

    m_buttonBox->addButton(QDialogButtonBox::Yes);
    m_buttonBox->addButton(QDialogButtonBox::No);
    m_buttonBox->button(QDialogButtonBox::No)->setFocus();
    connect(m_buttonBox->button(QDialogButtonBox::No), &QPushButton::clicked, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Yes), &QPushButton::clicked, this, &QDialog::accept);
  }

  updateModeButton(false);
  m_buttonBox->button(QDialogButtonBox::Help)->setCheckable(true);
  m_buttonBox->button(QDialogButtonBox::Help)->setIcon(QIcon());
  connect(
      m_buttonBox->button(QDialogButtonBox::Help), &QPushButton::toggled, this, &FingerprintDialog::togglePreviewMode
  );

  adjustSize();
  setFixedSize(size());
}

QLayout *FingerprintDialog::makeLocalLayout(const Fingerprint &localFingerprint)
{
  m_localPreview = new FingerprintPreview(this, localFingerprint);

  auto layout = new QVBoxLayout();
  layout->addWidget(m_localPreview, 0, Qt::AlignTop | Qt::AlignHCenter);
  return layout;
}

QLayout *FingerprintDialog::makeCompareLayout(
    const Fingerprint &localFingerprint, bool isServer, const Fingerprint &remoteFingerprint
)
{
  const auto serverText = tr("Server Fingerprint");
  const auto clientText = tr("Client Fingerprint");

  m_localPreview = new FingerprintPreview(this, localFingerprint, isServer ? serverText : clientText, false);
  m_remotePreview = new FingerprintPreview(this, remoteFingerprint, isServer ? clientText : serverText, false);

  auto fpLayout = new QHBoxLayout();
  fpLayout->setAlignment(Qt::AlignTop);
  if (isServer) {
    fpLayout->addWidget(m_localPreview, 0, Qt::AlignHCenter);
    fpLayout->addWidget(m_remotePreview, 0, Qt::AlignHCenter);
  } else {
    fpLayout->addWidget(m_remotePreview, 0, Qt::AlignHCenter);
    fpLayout->addWidget(m_localPreview, 0, Qt::AlignHCenter);
  }
  return fpLayout;
}

void FingerprintDialog::togglePreviewMode(bool hashMode)
{
  m_localPreview->toggleMode(hashMode);
  if (m_remotePreview)
    m_remotePreview->toggleMode(hashMode);
  updateModeButton(hashMode);
}

void FingerprintDialog::updateModeButton(bool hashMode) const
{
  const auto text = hashMode ? tr("Show image") : tr("Show hash");
  const auto toolTip = hashMode ? tr("Display the fingerprint as an image") : tr("Display the fingerprint as a hash");
  m_buttonBox->button(QDialogButtonBox::Help)->setText(text);
  m_buttonBox->button(QDialogButtonBox::Help)->setToolTip(toolTip);
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FingerprintPreview.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>

#include <net/SecureUtils.h>

FingerprintPreview::FingerprintPreview(QWidget *parent, const QList<deskflow::FingerprintData> &fingerprints)
    : QFrame(parent)
{
  setFrameShape(QFrame::StyledPanel);
  setFrameStyle(QFrame::Sunken);
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  QString sha1String;
  QString sha256String;
  QString sha256Art;

  for (const auto &fingerprint : fingerprints) {
    if (fingerprint.algorithm == "sha1") {
      sha1String = QString::fromStdString(deskflow::formatSSLFingerprint(fingerprint.data));
    }

    if (fingerprint.algorithm == "sha256") {
      sha256String = QString::fromStdString(deskflow::formatSSLFingerprintColumns(fingerprint.data));
      sha256Art = QString::fromStdString(deskflow::generateFingerprintArt(fingerprint.data));
    }
  }

  auto labelSha1 = new QLabel(QStringLiteral("SHA1:"), this);
  labelSha1->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  auto lblSha1 = new QLabel(sha1String, this);
  lblSha1->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  lblSha1->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto sha1Layout = new QHBoxLayout();
  sha1Layout->addWidget(labelSha1);
  sha1Layout->addWidget(lblSha1);

  auto frameSha1 = new QFrame(this);
  frameSha1->setFrameShape(QFrame::StyledPanel);
  frameSha1->setFrameStyle(QFrame::Sunken);
  frameSha1->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  frameSha1->setLayout(sha1Layout);

  auto labelSha256 = new QLabel(QStringLiteral("SHA256:"), this);
  labelSha256->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  auto lblSha256String = new QLabel(sha256String, this);
  lblSha256String->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
  lblSha256String->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto lblSha256Art = new QLabel(sha256Art, this);
  lblSha256Art->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
  lblSha256Art->setTextInteractionFlags(Qt::TextSelectableByMouse);

  QFont f = font();
  f.setFamilies({"Hack", "Liberation Mono", "Monospace", "Andale Mono"});
  f.setStyleHint(QFont::Monospace);
  lblSha256Art->setFont(f);

  auto innersha256Layout = new QHBoxLayout();
  innersha256Layout->setContentsMargins(0, 0, 0, 0);
  innersha256Layout->addWidget(lblSha256String);
  innersha256Layout->addWidget(lblSha256Art);

  auto sha256Layout = new QVBoxLayout();
  sha256Layout->addWidget(labelSha256);
  sha256Layout->addLayout(innersha256Layout);

  auto frameSha256 = new QFrame(this);
  frameSha256->setFrameShape(QFrame::StyledPanel);
  frameSha256->setFrameStyle(QFrame::Sunken);
  frameSha256->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  frameSha256->setLayout(sha256Layout);

  auto layout = new QVBoxLayout();
  layout->addWidget(frameSha1);
  layout->addWidget(frameSha256);

  setLayout(layout);

  if (sha1String.isEmpty()) {
    frameSha1->setVisible(false);
  }

  if (sha256String.isEmpty()) {
    frameSha256->setVisible(false);
  }

  adjustSize();
  setFixedSize(size());
}

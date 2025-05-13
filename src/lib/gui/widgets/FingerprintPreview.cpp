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

FingerprintPreview::FingerprintPreview(QWidget *parent, const Fingerprint &fingerprint) : QFrame(parent)
{
  setFrameShape(QFrame::StyledPanel);
  setFrameStyle(QFrame::Sunken);
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  setLayout(fingerprint.type == Fingerprint::Type::SHA256 ? sha256Layout(fingerprint) : emptyLayout());

  adjustSize();
  setFixedSize(size());
}

QLayout *FingerprintPreview::emptyLayout()
{
  auto *label = new QLabel(tr("Invalid hash format"));
  auto *layout = new QHBoxLayout();
  layout->addWidget(label);
  return layout;
}

QLayout *FingerprintPreview::sha256Layout(const Fingerprint &fingerprint)
{
  auto labelSha256 = new QLabel(QStringLiteral("SHA256:"), this);
  labelSha256->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  auto lblSha256String = new QLabel(deskflow::formatSSLFingerprintColumns(fingerprint.data), this);
  lblSha256String->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
  lblSha256String->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto lblSha256Art = new QLabel(deskflow::generateFingerprintArt(fingerprint.data), this);
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
  layout->addWidget(frameSha256);

  return layout;
}

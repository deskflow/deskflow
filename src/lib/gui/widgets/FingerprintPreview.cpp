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

FingerprintPreview::FingerprintPreview(
    QWidget *parent, const Fingerprint &fingerprint, const QString &titleText, bool hashMode
)
    : QFrame(parent)
{
  setFrameShape(QFrame::StyledPanel);
  setFrameStyle(QFrame::Sunken);
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  setLayout(
      fingerprint.type == Fingerprint::Type::SHA256 ? sha256Layout(fingerprint, titleText, hashMode) : emptyLayout()
  );
  adjustSize();
  setFixedSize(size());
}

void FingerprintPreview::toggleMode(bool hashMode)
{
  if (m_lblHash)
    m_lblHash->setVisible(hashMode);
  if (m_lblArt)
    m_lblArt->setVisible(!hashMode);
}

QLayout *FingerprintPreview::emptyLayout()
{
  auto *label = new QLabel(tr("Invalid hash format"));
  auto *layout = new QHBoxLayout();
  layout->addWidget(label);
  return layout;
}

QLayout *FingerprintPreview::sha256Layout(const Fingerprint &fingerprint, const QString &titleText, bool hashMode)
{
  auto labelTitle = new QLabel(titleText, this);
  auto f = font();
  f.setBold(true);
  f.setItalic(true);
  labelTitle->setFont(f);
  labelTitle->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
  labelTitle->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  labelTitle->setVisible(!titleText.isEmpty());

  m_lblHash = new QLabel(deskflow::formatSSLFingerprintColumns(fingerprint.data), this);
  m_lblHash->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
  m_lblHash->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_lblHash->setVisible(hashMode);

  m_lblArt = new QLabel(deskflow::generateFingerprintArt(fingerprint.data), this);
  m_lblArt->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
  m_lblArt->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_lblArt->setVisible(!hashMode);

  f = font();
  f.setFamilies({"Hack", "Liberation Mono", "Monospace", "Andale Mono"});
  f.setStyleHint(QFont::Monospace);
  m_lblArt->setFont(f);

  auto innersha256Layout = new QHBoxLayout();
  innersha256Layout->setContentsMargins(0, 0, 0, 0);
  innersha256Layout->addWidget(m_lblHash);
  innersha256Layout->addWidget(m_lblArt);

  auto sha256Layout = new QVBoxLayout();
  sha256Layout->setContentsMargins(0, 0, 0, 0);
  sha256Layout->addWidget(labelTitle);
  sha256Layout->addLayout(innersha256Layout);
  sha256Layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));

  auto frameSha256 = new QFrame(this);
  frameSha256->setFrameShape(QFrame::StyledPanel);
  frameSha256->setFrameStyle(QFrame::Sunken);
  frameSha256->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  frameSha256->setLayout(sha256Layout);

  auto layout = new QVBoxLayout();
  layout->addWidget(frameSha256);

  return layout;
}

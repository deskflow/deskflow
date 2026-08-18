/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HelpDialog.h"
#include <common/Constants.h>

#include <QDesktopServices>
#include <QEvent>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <qfile.h>

HelpDialog::HelpDialog(QWidget *parent, const QUrl &source)
    : QDialog(parent),
      m_initialSource{source},
      m_btnBack{new QPushButton(this)},
      m_btnHome{new QPushButton(this)},
      m_browser{new QTextBrowser(this)},
      m_btnClose{new QPushButton(this)}
{
  setWindowIcon(QIcon::fromTheme(QStringLiteral("question")));

  m_btnBack->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoPrevious));
  m_btnBack->setMaximumWidth(m_btnBack->height());
  m_btnBack->setShortcut(QKeySequence::Back);
  m_btnBack->setEnabled(false);
  m_btnHome->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoHome));
  m_btnHome->setMaximumWidth(m_btnHome->height());
  m_btnHome->setEnabled(false);
  m_btnClose->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::WindowClose));
  m_btnClose->setDefault(true);
  m_btnClose->setFocus();

  updateText();

  m_browser->setMinimumSize(650, 500);
  m_browser->setOpenExternalLinks(false);
  m_browser->setOpenLinks(false);

  if (!source.isLocalFile()) {
    QDesktopServices::openUrl(source);
  } else {
    m_browser->setSource(source);
  }

  auto topLayout = new QHBoxLayout;
  topLayout->addWidget(m_btnBack);
  topLayout->addWidget(m_btnHome);
  topLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
  auto layout = new QVBoxLayout;
  layout->addLayout(topLayout);
  layout->addWidget(m_browser);
  layout->addWidget(m_btnClose);
  setLayout(layout);
  adjustSize();

  connect(m_browser, &QTextBrowser::anchorClicked, this, &HelpDialog::linkClicked);
  connect(m_browser, &QTextBrowser::historyChanged, this, &HelpDialog::historyChanged);
  connect(m_btnBack, &QPushButton::clicked, m_browser, &QTextBrowser::backward);
  connect(m_btnHome, &QPushButton::clicked, m_browser, &QTextBrowser::home);
  connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
}

void HelpDialog::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  if (e->type() == QEvent::LanguageChange) {
    updateText();
  }
}

void HelpDialog::updateText()
{
  setWindowTitle(tr("%1 Help").arg(kAppName));
  m_btnClose->setText(tr("Close"));
}

void HelpDialog::historyChanged()
{
  m_btnBack->setEnabled(m_browser->isBackwardAvailable());
  m_btnHome->setEnabled(m_browser->source() != m_initialSource);
}

void HelpDialog::linkClicked(const QUrl &url)
{
  if (!url.isLocalFile())
    QDesktopServices::openUrl(url);
  else
    m_browser->setSource(url);
}

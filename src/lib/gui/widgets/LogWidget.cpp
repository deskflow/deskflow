/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LogWidget.h"
#include "common/PlatformInfo.h"

#include <gui/Logger.h>

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QTextDocument>
#include <QVBoxLayout>

LogWidget::LogWidget(QWidget *parent)
    : QWidget{parent},
      m_textLog{new QPlainTextEdit(this)},
      m_searchBar{new QLineEdit(this)}
{
  m_textLog->setReadOnly(true);
  m_textLog->setMaximumBlockCount(10000);
  m_textLog->setLineWrapMode(QPlainTextEdit::NoWrap);

  // setup the log font
  m_textLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  if (deskflow::platform::isMac()) {
    auto f = m_textLog->font();
    f.setPixelSize(12);
    m_textLog->setFont(f);
  }

  // Setup search bar
  m_searchBar->setPlaceholderText(tr("Search log..."));
  m_searchBar->setVisible(false);
  m_searchBar->setClearButtonEnabled(true);

  auto btnPrev = new QPushButton(QIcon::fromTheme(QIcon::ThemeIcon::GoUp), QString(), this);
  btnPrev->setToolTip(tr("Find previous"));
  btnPrev->setFlat(true);
  btnPrev->setVisible(false);

  auto btnNext = new QPushButton(QIcon::fromTheme(QIcon::ThemeIcon::GoDown), QString(), this);
  btnNext->setToolTip(tr("Find next"));
  btnNext->setFlat(true);
  btnNext->setVisible(false);

  auto btnClose = new QPushButton(QIcon::fromTheme("view-close"), QString(), this);
  btnClose->setToolTip(tr("Close search"));
  btnClose->setFlat(true);
  btnClose->setVisible(false);

  auto searchLayout = new QHBoxLayout;
  searchLayout->setContentsMargins(0, 0, 0, 0);
  searchLayout->addWidget(m_searchBar);
  searchLayout->addWidget(btnPrev);
  searchLayout->addWidget(btnNext);
  searchLayout->addWidget(btnClose);

  auto layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(searchLayout);
  layout->addWidget(m_textLog);

  setLayout(layout);

  // Ctrl+F shortcut to toggle search bar
  auto shortcut = new QShortcut(QKeySequence::Find, this);
  connect(shortcut, &QShortcut::activated, this, [this, btnPrev, btnNext, btnClose]() {
    bool visible = !m_searchBar->isVisible();
    m_searchBar->setVisible(visible);
    btnPrev->setVisible(visible);
    btnNext->setVisible(visible);
    btnClose->setVisible(visible);
    if (visible) {
      m_searchBar->setFocus();
      m_searchBar->selectAll();
    }
  });

  // Close button hides search
  connect(btnClose, &QPushButton::clicked, this, [this, btnPrev, btnNext, btnClose]() {
    m_searchBar->setVisible(false);
    btnPrev->setVisible(false);
    btnNext->setVisible(false);
    btnClose->setVisible(false);
  });

  // Search on Enter or text change
  connect(m_searchBar, &QLineEdit::returnPressed, this, &LogWidget::findNext);
  connect(btnNext, &QPushButton::clicked, this, &LogWidget::findNext);
  connect(btnPrev, &QPushButton::clicked, this, &LogWidget::findPrevious);

  connect(
      deskflow::gui::Logger::instance(), &deskflow::gui::Logger::newLine, m_textLog, &QPlainTextEdit::appendPlainText
  );
}

void LogWidget::appendLine(const QString &msg)
{
  m_textLog->appendPlainText(msg);
}

void LogWidget::findNext()
{
  if (!m_searchBar->text().isEmpty()) {
    if (!m_textLog->find(m_searchBar->text())) {
      // Wrap around to beginning
      m_textLog->moveCursor(QTextCursor::Start);
      m_textLog->find(m_searchBar->text());
    }
  }
}

void LogWidget::findPrevious()
{
  if (!m_searchBar->text().isEmpty()) {
    if (!m_textLog->find(m_searchBar->text(), QTextDocument::FindBackward)) {
      // Wrap around to end
      m_textLog->moveCursor(QTextCursor::End);
      m_textLog->find(m_searchBar->text(), QTextDocument::FindBackward);
    }
  }
}

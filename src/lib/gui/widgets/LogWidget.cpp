/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LogWidget.h"
#include "common/PlatformInfo.h"

#include <gui/Logger.h>

#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QTimer>
#include <QVBoxLayout>

LogWidget::LogWidget(QWidget *parent)
    : QWidget{parent},
      m_textLog{new QPlainTextEdit(this)},
      m_copyThrottle{new QTimer(this)}
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

  auto layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_textLog);

  setLayout(layout);

  m_textLog->installEventFilter(this);

  connect(
      deskflow::gui::Logger::instance(), &deskflow::gui::Logger::newLine, m_textLog, &QPlainTextEdit::appendPlainText
  );
}

bool LogWidget::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == m_textLog && event->type() == QEvent::KeyPress) {
    auto *keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->matches(QKeySequence::Copy)) {
      if (m_copyThrottle->isActive()) {
        return true;
      }
      m_copyThrottle->start(200);
    }
  }
  return false;
}

void LogWidget::appendLine(const QString &msg)
{
  m_textLog->appendPlainText(msg);
}

void LogWidget::findNext(const QString &text)
{
  if (text.isEmpty())
    return;

  if (!m_textLog->find(text)) {
    m_textLog->moveCursor(QTextCursor::Start);
    m_textLog->find(text);
  }
}

void LogWidget::findPrevious(const QString &text)
{
  if (text.isEmpty())
    return;

  if (!m_textLog->find(text, QTextDocument::FindBackward)) {
    m_textLog->moveCursor(QTextCursor::End);
    m_textLog->find(text, QTextDocument::FindBackward);
  }
}

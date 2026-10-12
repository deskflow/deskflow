/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LogWidget.h"

#include <gui/Logger.h>
#include <gui/StyleUtils.h>

#include <QPlainTextEdit>
#include <QScrollBar>
#include <QVBoxLayout>

LogWidget::LogWidget(QWidget *parent) : QWidget{parent}, m_textLog{new QPlainTextEdit(this)}
{
  m_textLog->setReadOnly(true);
  m_textLog->setMaximumBlockCount(10000);
  m_textLog->setLineWrapMode(QPlainTextEdit::NoWrap);
  m_textLog->setFont(fixedFont());

  auto layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_textLog);

  setLayout(layout);

  connect(
      deskflow::gui::Logger::instance(), &deskflow::gui::Logger::newLine, m_textLog, &QPlainTextEdit::appendPlainText
  );

  // only user scrolling counts, Qt shifts the value itself when trimming lines or while hidden
  auto sb = m_textLog->verticalScrollBar();
  connect(sb, &QScrollBar::actionTriggered, this, [this, sb] { m_followTail = sb->sliderPosition() == sb->maximum(); });
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

bool LogWidget::isFollowingTail() const
{
  return m_followTail;
}

void LogWidget::scrollToBottom() const
{
  auto sb = m_textLog->verticalScrollBar();
  sb->setValue(sb->maximum());
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LogWidget.h"
#include "common/PlatformInfo.h"

#include <gui/Logger.h>

#include <QPlainTextEdit>
#include <QVBoxLayout>

LogWidget::LogWidget(QWidget *parent) : QWidget{parent}, m_textLog{new QPlainTextEdit(this)}
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

  connect(
      deskflow::gui::Logger::instance(), &deskflow::gui::Logger::newLine, m_textLog, &QPlainTextEdit::appendPlainText
  );
}

void LogWidget::appendLine(const QString &msg)
{
  m_textLog->appendPlainText(msg);
}

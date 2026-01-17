/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;
class QPlainTextEdit;

class LogWidget : public QWidget
{
  Q_OBJECT
public:
  explicit LogWidget(QWidget *parent = nullptr);
  void appendLine(const QString &msg);

private Q_SLOTS:
  void findNext();
  void findPrevious();

private:
  QPlainTextEdit *m_textLog = nullptr;
  QLineEdit *m_searchBar = nullptr;
};

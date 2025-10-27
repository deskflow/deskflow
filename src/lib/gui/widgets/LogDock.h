/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDockWidget>

class LogWidget;
class QLabel;
class QPushButton;

class LogDock : public QDockWidget
{
  Q_OBJECT
public:
  explicit LogDock(QWidget *parent = nullptr);
  void appendLine(const QString &msg);
  void setFloating(bool floating);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void changeEvent(QEvent *e) override;

private:
  LogWidget *m_textLog = nullptr;
  QPushButton *m_btnClose = nullptr;
  QPushButton *m_btnFloat = nullptr;
  QLabel *m_lblTitle = nullptr;
};

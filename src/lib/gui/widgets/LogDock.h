/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDockWidget>

class LogWidget;
class QLabel;
class QToolButton;

class LogDock : public QDockWidget
{
  Q_OBJECT
public:
  explicit LogDock(QWidget *parent = nullptr);
  void appendLine(const QString &msg);
  void setFloating(bool floating);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  LogWidget *m_textLog = nullptr;
  QToolButton *m_btnClose = nullptr;
  QToolButton *m_btnFloat = nullptr;
  QLabel *m_lblTitle = nullptr;
};

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QWidget>

class QPushButton;
class QLineEdit;

class SearchWidget : public QWidget
{
  Q_OBJECT
public:
  explicit SearchWidget(QWidget *parent = nullptr);

Q_SIGNALS:
  void findNext(const QString &text);
  void findPrevious(const QString &text);

protected:
  void changeEvent(QEvent *e) override;

private:
  void toggleVisible(bool visible = false);
  void setText();
  void next();
  void previous();
  QPushButton *m_btnToggle = nullptr;
  QPushButton *m_btnNext = nullptr;
  QPushButton *m_btnPrev = nullptr;
  QLineEdit *m_searchLine = nullptr;
};

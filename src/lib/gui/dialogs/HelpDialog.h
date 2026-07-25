/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDialog>
#include <QPushButton>
#include <QTextBrowser>
#include <QUrl>

class HelpDialog : public QDialog
{
  Q_OBJECT

public:
  explicit HelpDialog(QWidget *parent = nullptr, const QUrl &source = {});
  ~HelpDialog() override = default;

protected:
  void changeEvent(QEvent *e) override;

private:
  void updateText();
  void historyChanged();
  void linkClicked(const QUrl &url);
  QUrl m_initialSource;
  QPushButton *m_btnBack = nullptr;
  QPushButton *m_btnHome = nullptr;
  QTextBrowser *m_browser = nullptr;
  QPushButton *m_btnClose = nullptr;
};

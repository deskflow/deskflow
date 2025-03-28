/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDialog>

class QPushButton;
class QLabel;

namespace Ui {
class AddClientDialog;
}

enum
{
  kAddClientRight,
  kAddClientLeft,
  kAddClientUp,
  kAddClientDown,
  kAddClientOther,
  kAddClientIgnore
};

class AddClientDialog : public QDialog
{
  Q_OBJECT
public:
  AddClientDialog(const QString &clientName, QWidget *parent = 0);
  ~AddClientDialog() override;

  int addResult()
  {
    return m_AddResult;
  }

private slots:
  void handleButtonLeft();
  void handleButtonUp();
  void handleButtonRight();
  void handleButtonDown();
  void handleButtonAdvanced();

private:
  std::unique_ptr<Ui::AddClientDialog> ui;
  QPushButton *m_pButtonLeft;
  QPushButton *m_pButtonUp;
  QPushButton *m_pButtonRight;
  QPushButton *m_pButtonDown;
  QLabel *m_pLabelCenter;
  int m_AddResult;
};

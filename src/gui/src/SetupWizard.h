/*
 * Deskflow -- mouse and keyboard sharing utility
 *
 * SPDX-FileCopyrightText: Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: Copyright (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0
 */

#pragma once

#include <QDialog>

class AppConfig;
class QLabel;
class QLineEdit;

class SetupWizard : public QDialog
{
  Q_OBJECT

public:
  explicit SetupWizard(AppConfig &appConfig);
  ~SetupWizard() = default;

protected:
  void accept() override;
  void reject() override;

private:
  void nameChanged(const QString &error);

  AppConfig &m_appConfig;
  QLabel *m_lblError = nullptr;
  QLineEdit *m_lineName = nullptr;
  QPushButton *m_btnApply = nullptr;
};

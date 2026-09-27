/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDialog>

class QWidget;
class QString;

class Computer;
class ComputerList;

namespace Ui {
class ComputerSettingsDialog;
}

class ComputerSettingsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ComputerSettingsDialog(
      QWidget *parent, Computer *computer = nullptr, const ComputerList *computers = nullptr
  );
  ~ComputerSettingsDialog() override;

public Q_SLOTS:
  void accept() override;

private Q_SLOTS:
  void addAlias();
  void removeAlias() const;
  void checkNewAliasName(const QString &text);
  void aliasSelected();

private:
  std::unique_ptr<Ui::ComputerSettingsDialog> ui;
  Computer *m_computer;
};

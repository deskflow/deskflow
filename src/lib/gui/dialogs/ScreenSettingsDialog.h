/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDialog>

class QWidget;
class QString;

class Screen;
class ScreenList;

namespace Ui {
class ScreenSettingsDialog;
}

class ScreenSettingsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ScreenSettingsDialog(QWidget *parent, Screen *screen = nullptr, const ScreenList *screens = nullptr);
  ~ScreenSettingsDialog() override;

public Q_SLOTS:
  void accept() override;

private Q_SLOTS:
  void on_btnAddAlias_clicked();
  void on_btnRemoveAlias_clicked();
  void on_lineAddAlias_textChanged(const QString &text);
  void on_listAliases_itemSelectionChanged();

private:
  std::unique_ptr<Ui::ScreenSettingsDialog> ui;
  Screen *m_screen;
};

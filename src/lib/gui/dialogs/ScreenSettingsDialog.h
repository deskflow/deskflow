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
  ScreenSettingsDialog(QWidget *parent, Screen *pScreen = nullptr, const ScreenList *pScreens = nullptr);
  ~ScreenSettingsDialog() override;

public slots:
  void accept() override;

private slots:
  void on_m_pButtonAddAlias_clicked();
  void on_m_pButtonRemoveAlias_clicked();
  void on_m_pLineEditAlias_textChanged(const QString &text);
  void on_m_pListAliases_itemSelectionChanged();

private:
  std::unique_ptr<Ui::ScreenSettingsDialog> ui_;
  Screen *m_pScreen;
};

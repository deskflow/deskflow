/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

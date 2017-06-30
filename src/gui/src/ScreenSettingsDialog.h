/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#if !defined(SCREENSETTINGSDIALOG__H)

#define SCREENSETTINGSDIALOG__H

#include <QDialog>

#include "ui_ScreenSettingsDialogBase.h"

class QWidget;
class QString;

class Screen;

class ScreenSettingsDialog : public QDialog,
                             public Ui::ScreenSettingsDialogBase {
    Q_OBJECT

public:
    ScreenSettingsDialog (QWidget* parent, Screen* pScreen = NULL);

public slots:
    void accept ();

private slots:
    void on_m_pButtonAddAlias_clicked ();
    void on_m_pButtonRemoveAlias_clicked ();
    void on_m_pLineEditAlias_textChanged (const QString& text);
    void on_m_pListAliases_itemSelectionChanged ();

private:
    Screen* m_pScreen;
};

#endif

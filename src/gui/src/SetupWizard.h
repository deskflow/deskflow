/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#include "ui_SetupWizardBase.h"
#include "SynergyLocale.h"

#include <QWizard>
#include <QNetworkAccessManager>

class MainWindow;

class SetupWizard : public QWizard, public Ui::SetupWizardBase {
    Q_OBJECT
public:
    enum { kMaximiumLoginAttemps = 3 };

public:
    SetupWizard (MainWindow& mainWindow, bool startMain);
    virtual ~SetupWizard ();
    bool validateCurrentPage ();

protected:
    void changeEvent (QEvent* event);
    void accept ();
    void reject ();

private:
    MainWindow& m_MainWindow;
    bool m_StartMain;
    SynergyLocale m_Locale;

private slots:
    void on_m_pComboLanguage_currentIndexChanged (int index);
};

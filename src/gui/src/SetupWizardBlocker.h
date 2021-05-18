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
#include "ui_SetupWizardBlocker.h"
#include <QDialog>

class MainWindow;

class SetupWizardBlocker : public QDialog, public Ui::SetupWizardBlocker
{
    Q_OBJECT

public:
    enum class qBlockerType
    {
        waylandDetected
    };
    explicit SetupWizardBlocker(MainWindow& mainWindow, qBlockerType type);

protected:
    void onlineSupport();
    void cancel();

private:
    MainWindow& m_MainWindow;
};

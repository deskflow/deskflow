/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

#include <QWizard>
#include "ui_SetupWizardBase.h"
#include "CryptoMode.h"

class MainWindow;

class SetupWizard : public QWizard, public Ui::SetupWizardBase
{
	Q_OBJECT
public:
	SetupWizard(MainWindow& mainWindow, bool startMain);
	virtual ~SetupWizard();
	bool validateCurrentPage();
protected slots:
	void handleFinished();
private:
	MainWindow& m_MainWindow;
	bool m_StartMain;
	CryptoMode parseCryptoMode(const QString& s);
private slots:
    void on_m_pComboCryptoMode_currentIndexChanged(int index);
};

#pragma once

#include <QWizard>

#include "ui_SetupWizardBase.h"

class MainWindow;

class SetupWizard : public QWizard, public Ui::SetupWizardBase
{
	Q_OBJECT
public:
	SetupWizard(MainWindow& mainWindow);
	virtual ~SetupWizard();
private:
	MainWindow& m_MainWindow;
protected slots:
	void showMain();
};

#pragma once

#include <QWizard>

#include "ui_SetupWizardBase.h"

class MainWindow;

class SetupWizard : public QWizard, public Ui::SetupWizardBase
{
	Q_OBJECT
public:
	SetupWizard(MainWindow& mainWindow, bool startMain);
	virtual ~SetupWizard();
	bool validateCurrentPage();
protected slots:
	void handlefinished();
private:
	MainWindow& m_MainWindow;
	bool m_StartMain;
};

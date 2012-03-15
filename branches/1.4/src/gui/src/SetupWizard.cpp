#include "SetupWizard.h"
#include "MainWindow.h"

SetupWizard::SetupWizard(MainWindow& mainWindow) :
	m_MainWindow(mainWindow)
{
	setupUi(this);

	connect(this, SIGNAL(finished(int)), this, SLOT(showMain()));
}

SetupWizard::~SetupWizard()
{
}

void SetupWizard::showMain()
{
	close();
	m_MainWindow.start();
}


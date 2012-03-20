#include "SetupWizard.h"
#include "MainWindow.h"
#include "WindowsServices.h"

#include <QMessageBox>

SetupWizard::SetupWizard(MainWindow& mainWindow, bool startMain) :
	m_MainWindow(mainWindow),
	m_StartMain(startMain)
{
	setupUi(this);

#if !defined(Q_OS_WIN)
	m_pServiceRadioButton->setEnabled(false);
	m_pServiceRadioButton->setText(tr("Service (Windows only)"));
	m_pServiceLabel->setEnabled(false);
	m_pDesktopRadioButton->setChecked(true);
#endif

	connect(this, SIGNAL(finished(int)), this, SLOT(handlefinished()));
	connect(m_pServerRadioButton, SIGNAL(toggled(bool)), m_MainWindow.m_pGroupServer, SLOT(setChecked(bool)));
	connect(m_pClientRadioButton, SIGNAL(toggled(bool)), m_MainWindow.m_pGroupClient, SLOT(setChecked(bool)));
}

SetupWizard::~SetupWizard()
{
}

bool SetupWizard::validateCurrentPage()
{	
	QMessageBox message;
	message.setWindowTitle(tr("Setup Synergy"));
	message.setIcon(QMessageBox::Information);

	bool result = false;
	if (currentPage() == m_pNodePage)
	{
		result = m_pClientRadioButton->isChecked() ||
				 m_pServerRadioButton->isChecked();

		if (!result)
		{
			message.setText(tr("Please select an option."));
			message.exec();
		}
	}
	else if(currentPage() == m_pStartupPage)
	{
		result = m_pServiceRadioButton->isChecked() ||
				 m_pDesktopRadioButton->isChecked() ||
				 m_pNoneRadioButton->isChecked();

		if (!result)
		{
			message.setText(tr("Please select an option."));
			message.exec();
		}
	}
	return result;
}

void SetupWizard::handlefinished()
{
	close();

#if defined(Q_OS_WIN)
	// HACK: use existing windows service manager *window* to
	// install services. this should really be in a separate
	// non-window class.
	WindowsServices windowsServices(&m_MainWindow, m_MainWindow.appConfig());

	// install client and server services if needed.
	if (m_pServiceRadioButton->isChecked())
	{
		if (m_pServerRadioButton->isChecked())
		{
			windowsServices.installServer();
		}

		if (m_pClientRadioButton->isChecked())
		{
			windowsServices.installClient();
		}
	}
	else
	{
		// remove client and server services if they exist.
		if (m_MainWindow.appConfig().clientService())
		{
			windowsServices.uninstallClient();
		}

		if (m_MainWindow.appConfig().serverService())
		{
			windowsServices.uninstallServer();
		}
	}
#endif

	if (m_StartMain)
	{
		m_MainWindow.start();
	}
}


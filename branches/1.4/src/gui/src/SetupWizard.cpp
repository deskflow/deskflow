#include "SetupWizard.h"
#include "MainWindow.h"

#include <QMessageBox>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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

	if (m_StartMain)
	{
		m_MainWindow.start();
	}
}


void SetupWizard::on_pushButton_clicked()
{
	// TODO: put this in an IPC client class.
#if defined(Q_OS_WIN)

	const WCHAR* name = L"\\\\.\\pipe\\Synergy";
	const char* message = "hello world";

	HANDLE pipe = CreateFile(
		name, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL);

	DWORD dwMode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(
		pipe,    // pipe handle
		&dwMode,  // new pipe mode
		NULL,     // don't set maximum bytes
		NULL);    // don't set maximum time

	DWORD written;
	WriteFile(
	   pipe,                  // pipe handle
	   message,             // message
	   strlen(message),              // message length
	   &written,             // bytes written
	   NULL);                  // not overlapped

	CloseHandle(pipe);

#endif
}

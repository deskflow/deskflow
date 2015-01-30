#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>

#include "ui_LoginWindowBase.h"

class MainWindow;
class SetupWizard;
class LoginAuth;

class LoginWindow : public QMainWindow, public Ui::LoginWindow
{
    Q_OBJECT
public:
	LoginWindow(MainWindow* mainWindow,
		SetupWizard* setupWizard,
		bool wizardShouldRun,
		QWidget *parent = 0);
    ~LoginWindow();

	void setLoginResult(int result) { m_LoginResult = result; }
	void setError(QString error) { m_Error = error; }

protected:
    void changeEvent(QEvent *e);
	void closeEvent(QCloseEvent *event);

private slots:
	void on_m_pPushButtonCancel_clicked();
	void on_m_pPushButtonLogin_clicked();
	void showNext();

private:
	bool validEmailPassword();
private:
	MainWindow* m_pMainWindow;
	SetupWizard* m_pSetupWizard;
	bool m_WizardShouldRun;
	LoginAuth* m_pLoginAuth;
	int m_LoginResult;
	QString m_Error;

};

#endif // LOGINWINDOW_H

#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include "ui_LoginDialogBase.h"

#include <QDialog>

class MainWindow;
class SetupWizard;
class LoginAuth;
class AppConfig;

class LoginDialog : public QDialog, public Ui::LoginDialog {
    Q_OBJECT
public:
	LoginDialog(MainWindow* mainWindow,
				SetupWizard* setupWizard,
				bool wizardShouldRun,
				QWidget *parent = 0);
    ~LoginDialog();

	void setLoginResult(int result) { m_LoginResult = result; }
	void setEditionType(int type) { m_EditionType = type; }
	void setError(QString error) { m_Error = error; }

protected:
	void changeEvent(QEvent *e);
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent *e);

private slots:
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
	int m_EditionType;
	QString m_Error;
	AppConfig& m_AppConfig;
};

#endif // LOGINDIALOG_H

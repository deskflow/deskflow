#ifndef WINDOWSSERVICES_H
#define WINDOWSSERVICES_H

#include "ui_WindowsServicesBase.h"

class QWidget;
class QProcess;
class QPushButton;
class QProcess;

class AppConfig;
class MainWindow;
class LogDialog;

class WindowsServices : public QDialog, public Ui::WindowsServicesBase
{
	Q_OBJECT

	public:
		WindowsServices(QWidget* parent, AppConfig& appConfig);

	protected:
		AppConfig &appConfig() const { return m_appConfig; }
		MainWindow* mainWindow() const { return (MainWindow*)parent(); }
		QProcess*& process() { return m_process; }
		void runProc(const QString& app, const QStringList& args, QPushButton* button);

	private:
		QString m_app;
		AppConfig &m_appConfig;
		QProcess* m_process;
		LogDialog* m_log;

	private slots:
		void on_m_pUninstallClient_clicked();
		void on_m_pInstallClient_clicked();
		void on_m_pUninstallServer_clicked();
		void on_m_pInstallServer_clicked();
};

#endif // WINDOWSSERVICES_H

#ifndef PLUGINWIZARDPAGE_H
#define PLUGINWIZARDPAGE_H

#include "ui_PluginWizardPageBase.h"
#include <QWizardPage>

class WebClient;
class PluginManager;

class PluginWizardPage : public QWizardPage, public Ui::PluginWizardPage {

    Q_OBJECT

public:
    PluginWizardPage(QWidget *parent = 0);
    ~PluginWizardPage();

	void setFinished(bool b) { m_Finished = b; }
	void setEmail(QString e) { m_Email = e; }
	void setPassword(QString p) { m_Password = p; }

	bool isComplete() const;
	void initializePage();

protected:
    void changeEvent(QEvent *e);

protected slots:
	void showError(QString error);
	void queryPluginDone();
	void updateDownloadStatus();
	void finished();
	void generateCertificate();

private:
	void updateStatus(QString info);
	void downloadPlugins();
	void stopSpinning();

private:
	bool m_Finished;
	QString m_Email;
	QString m_Password;
	WebClient* m_pWebClient;
	PluginManager* m_pPluginManager;
	QThread* m_pPluginManagerThread;
};
#endif // PLUGINWIZARDPAGE_H

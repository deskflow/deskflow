#ifndef PLUGINWIZARDPAGE_H
#define PLUGINWIZARDPAGE_H

#include "ui_PluginWizardPageBase.h"
#include <QWizardPage>

class WebClient;

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
	void queryPluginDone();

private:
	void updateStatus(QString info);

private:
	bool m_Finished;
	WebClient* m_pWebClient;
	QString m_Email;
	QString m_Password;
};
#endif // PLUGINWIZARDPAGE_H

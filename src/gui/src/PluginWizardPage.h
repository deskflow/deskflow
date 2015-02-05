#ifndef PLUGINWIZARDPAGE_H
#define PLUGINWIZARDPAGE_H

#include "ui_PluginWizardPageBase.h"
#include <QWizardPage>

class PluginWizardPage : public QWizardPage, public Ui::PluginWizardPage {

    Q_OBJECT

public:
    PluginWizardPage(QWidget *parent = 0);
    ~PluginWizardPage();

	void setFinished(bool b) { m_Finished = b; }

	bool isComplete() const;
	void initializePage();

protected:
    void changeEvent(QEvent *e);

private:
	bool m_Finished;
};
#endif // PLUGINWIZARDPAGE_H

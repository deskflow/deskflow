#include "PluginWizardPage.h"
#include "ui_PluginWizardPageBase.h"

#include <QMovie>

PluginWizardPage::PluginWizardPage(QWidget *parent) :
	QWizardPage(parent),
	m_Finished(false)
{
	setupUi(this);

	QMovie *movie = new QMovie(":/res/image/spinning-wheel.gif");
	m_pLabelSpinning->setMovie(movie);
	movie->start();
}

PluginWizardPage::~PluginWizardPage()
{
}

void PluginWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
		retranslateUi(this);
        break;
    default:
        break;
    }
}

bool PluginWizardPage::isComplete() const
{
	return m_Finished;
}

void PluginWizardPage::initializePage()
{
	QWizardPage::initializePage();
	m_Finished = true;
	emit completeChanged();
}

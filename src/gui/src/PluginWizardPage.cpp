#include "PluginWizardPage.h"
#include "ui_PluginWizardPageBase.h"

#include "WebClient.h"

#include <QMovie>
#include <QThread>

PluginWizardPage::PluginWizardPage(QWidget *parent) :
	QWizardPage(parent),
	m_Finished(false),
	m_pWebClient(NULL)
{
	setupUi(this);

	QMovie *movie = new QMovie(":/res/image/spinning-wheel.gif");
	m_pLabelSpinning->setMovie(movie);
	movie->start();
}

PluginWizardPage::~PluginWizardPage()
{
	if (m_pWebClient != NULL) {
		delete m_pWebClient;
	}
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

void PluginWizardPage::queryPluginDone()
{
	QStringList plguinList = m_pWebClient->getPluginList();
	if (plguinList.isEmpty()) {
		if (!m_pWebClient->getLastError().isEmpty()) {
			updateStatus(m_pWebClient->getLastError());
			m_Finished = true;
			emit completeChanged();
		}
	}
	else {
		updateStatus(plguinList.at(0));
	}
}

void PluginWizardPage::updateStatus(QString info)
{
	m_pLabelStatus->setText(info);
}

bool PluginWizardPage::isComplete() const
{
	return m_Finished;
}

void PluginWizardPage::initializePage()
{
	QWizardPage::initializePage();
	if (m_pWebClient == NULL) {
		if (m_Email.isEmpty() ||
			m_Password.isEmpty()) {
			updateStatus("No plugin available.");
			//TODO: stop spinning icon
			m_Finished = true;
			emit completeChanged();

			return;
		}

		m_pWebClient = new WebClient();
		m_pWebClient->setEmail(m_Email);
		m_pWebClient->setPassword(m_Password);

		QThread* thread = new QThread;
		connect(m_pWebClient,
			SIGNAL(queryPluginDone()),
			this,
			SLOT(queryPluginDone()));

		connect(m_pWebClient, SIGNAL(queryPluginDone()), thread, SLOT(quit()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

		m_pWebClient->moveToThread(thread);
		thread->start();

		updateStatus("Querying plugin list...");
		QMetaObject::invokeMethod(m_pWebClient, "queryPluginList", Qt::QueuedConnection);
	}
}

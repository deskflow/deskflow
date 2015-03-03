/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PluginWizardPage.h"
#include "ui_PluginWizardPageBase.h"

#include "WebClient.h"
#include "PluginManager.h"

#include <QMovie>
#include <QThread>

PluginWizardPage::PluginWizardPage(AppConfig& appConfig, QWidget *parent) :
	QWizardPage(parent),
	m_Finished(false),
	m_pWebClient(NULL),
	m_pPluginManager(NULL),
	m_AppConfig(appConfig)
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

	if (m_pPluginManager != NULL) {
		delete m_pPluginManager;
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

void PluginWizardPage::showError(QString error)
{
	updateStatus(tr("Error: %1").arg(error));
	showFinished();
}

void PluginWizardPage::queryPluginDone()
{
	QStringList pluginList = m_pWebClient->getPluginList();
	if (pluginList.isEmpty()) {
		updateStatus(tr("Setup complete."));
		showFinished();
	}
	else {
		downloadPlugins();
	}
}

void PluginWizardPage::updateDownloadStatus()
{
	QStringList pluginList = m_pWebClient->getPluginList();
	int index = m_pPluginManager->downloadIndex();
	updateStatus(
		tr("Downloading '%1' plugin (%2/%3)...")
		.arg(pluginList.at(index + 1))
		.arg(index + 2)
		.arg(pluginList.size()));
}

void PluginWizardPage::finished()
{
	// TODO: we should check if ns plugin exists
	m_AppConfig.setCryptoEnabled(true);

	updateStatus(tr("Plugins installed successfully."));
	showFinished();
}

void PluginWizardPage::generateCertificate()
{
	connect(m_pPluginManager,
		SIGNAL(generateCertificateFinished()),
		this,
		SLOT(finished()));

	connect(m_pPluginManager,
		SIGNAL(generateCertificateFinished()),
		m_pPluginManagerThread,
		SLOT(quit()));

	updateStatus(tr("Generating SSL certificate..."));

	QMetaObject::invokeMethod(
		m_pPluginManager,
		"generateCertificate",
		Qt::QueuedConnection);
}

void PluginWizardPage::updateStatus(QString info)
{
	m_pLabelStatus->setText(info);
}

void PluginWizardPage::downloadPlugins()
{
	QStringList pluginList = m_pWebClient->getPluginList();
	m_pPluginManager = new PluginManager(pluginList);
	m_pPluginManagerThread = new QThread;

	connect(m_pPluginManager,
		SIGNAL(error(QString)),
		this,
		SLOT(showError(QString)));

	connect(m_pPluginManager,
		SIGNAL(downloadNext()),
		this,
		SLOT(updateDownloadStatus()));

	connect(m_pPluginManager,
		SIGNAL(downloadFinished()),
		this,
		SLOT(generateCertificate()));

	connect(m_pPluginManager,
		SIGNAL(error(QString)),
		m_pPluginManagerThread,
		SLOT(quit()));

	connect(m_pPluginManagerThread,
		SIGNAL(finished()),
		m_pPluginManagerThread,
		SLOT(deleteLater()));

	updateStatus(
		tr("Downloading plugin: %1 (1/%2)")
		.arg(pluginList.at(0))
		.arg(pluginList.size()));

	m_pPluginManager->moveToThread(m_pPluginManagerThread);
	m_pPluginManagerThread->start();

	QMetaObject::invokeMethod(
		m_pPluginManager,
		"downloadPlugins",
		Qt::QueuedConnection);
}

void PluginWizardPage::showFinished()
{
	m_pLabelSpinning->hide();
	m_Finished = true;
	emit completeChanged();
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
			updateStatus(tr("Setup complete."));
			showFinished();
			return;
		}

		m_pLabelSpinning->show();

		m_pWebClient = new WebClient();
		m_pWebClient->setEmail(m_Email);
		m_pWebClient->setPassword(m_Password);

		QThread* thread = new QThread;

		connect(m_pWebClient,
			SIGNAL(error(QString)),
			this,
			SLOT(showError(QString)));

		connect(m_pWebClient,
			SIGNAL(queryPluginDone()),
			this,
			SLOT(queryPluginDone()));

		connect(m_pWebClient,
			SIGNAL(queryPluginDone()),
			thread,
			SLOT(quit()));

		connect(m_pWebClient,
			SIGNAL(error(QString)),
			thread,
			SLOT(quit()));

		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

		m_pWebClient->moveToThread(thread);
		thread->start();

		updateStatus(tr("Getting plugin list..."));
		QMetaObject::invokeMethod(m_pWebClient, "queryPluginList", Qt::QueuedConnection);
	}
}

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
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

#include "SslCertificate.h"
#include "PluginManager.h"
#include "MainWindow.h"
#include "EditionType.h"

#include <QMovie>
#include <QThread>
#include <QTime>

PluginWizardPage::PluginWizardPage(MainWindow& mainWindow, QWidget *parent) :
	QWizardPage(parent),
	m_Finished(false),
	m_Edition(Unknown),
	m_pSslCertificate(NULL),
	m_mainWindow(mainWindow)
{
	setupUi(this);

	QMovie *movie = new QMovie(":/res/image/spinning-wheel.gif");
	m_pLabelSpinning->setMovie(movie);
	movie->start();

	m_pSslCertificate = new SslCertificate(this);
}

PluginWizardPage::~PluginWizardPage()
{
	delete m_pSslCertificate;
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

void PluginWizardPage::initializePage()
{
	QWizardPage::initializePage();

	if (m_Edition == Unknown ||
		m_Edition == Basic) {
		updateStatus(tr("Setup complete."));
		showFinished();
		return;
	}

	m_pLabelSpinning->show();

	QThread* thread = new QThread;

	connect(&m_PluginManager,
		SIGNAL(error(QString)),
		this,
		SLOT(showError(QString)));

	connect(&m_PluginManager,
		SIGNAL(info(QString)),
		this,
		SLOT(updateStatus(QString)));

	connect(&m_PluginManager,
		SIGNAL(queryPluginDone()),
		this,
		SLOT(queryPluginDone()));

	connect(&m_PluginManager,
		SIGNAL(queryPluginDone()),
		thread,
		SLOT(quit()));

	connect(&m_PluginManager,
		SIGNAL(error(QString)),
		thread,
		SLOT(quit()));

	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

	m_PluginManager.moveToThread(thread);
	thread->start();

	QMetaObject::invokeMethod(&m_PluginManager, "queryPluginList", Qt::QueuedConnection);
}

void PluginWizardPage::queryPluginDone()
{
	QStringList pluginList = m_PluginManager.getPluginList();
	if (pluginList.isEmpty()) {
		updateStatus(tr("Setup complete."));
		showFinished();
	}
	else {
		m_mainWindow.stopSynergy();
		copyPlugins();
		m_mainWindow.startSynergy();
	}
}

void PluginWizardPage::copyPlugins()
{
	m_pThread = new QThread;

	connect(&m_PluginManager,
		SIGNAL(copyFinished()),
		this,
		SLOT(generateCertificate()));

	connect(&m_PluginManager,
		SIGNAL(error(QString)),
		m_pThread,
		SLOT(quit()));

	connect(m_pThread,
		SIGNAL(finished()),
		m_pThread,
		SLOT(deleteLater()));

	updateStatus(
		tr("Copying plugins..."));

	m_PluginManager.moveToThread(m_pThread);
	m_pThread->start();

	QMetaObject::invokeMethod(
		&m_PluginManager,
		"copyPlugins",
		Qt::QueuedConnection);
}

void PluginWizardPage::generateCertificate()
{
	connect(m_pSslCertificate,
		SIGNAL(generateFinished()),
		this,
		SLOT(finished()));

	connect(m_pSslCertificate,
		SIGNAL(generateFinished()),
		m_pThread,
		SLOT(quit()));

	updateStatus(tr("Generating SSL certificate..."));

	QMetaObject::invokeMethod(
		m_pSslCertificate,
		"generateCertificate",
		Qt::QueuedConnection);
}

void PluginWizardPage::showError(QString error)
{
	updateStatus(tr("Error: %1").arg(error));
	showFinished();
}


void PluginWizardPage::updateStatus(QString info)
{
	m_pLabelStatus->setText(info);
}

void PluginWizardPage::finished()
{
	// TODO: we should check if ns plugin exists
	m_mainWindow.appConfig().setCryptoEnabled(true);

	updateStatus(tr("Plugins installed successfully."));
	showFinished();
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

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

#ifndef PLUGINWIZARDPAGE_H
#define PLUGINWIZARDPAGE_H

#include "AppConfig.h"

#include "ui_PluginWizardPageBase.h"
#include <QWizardPage>

class WebClient;
class PluginManager;
class SslCertificate;

class PluginWizardPage : public QWizardPage, public Ui::PluginWizardPage {

    Q_OBJECT

public:
	PluginWizardPage(AppConfig& appConfig, QWidget *parent = 0);
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
	void updateStatus(QString info);
	void queryPluginDone();
	void updateDownloadStatus();
	void finished();
	void generateCertificate();

private:
	void downloadPlugins();
	void showFinished();

private:
	bool m_Finished;
	QString m_Email;
	QString m_Password;
	WebClient* m_pWebClient;
	PluginManager* m_pPluginManager;
	SslCertificate* m_pSslCertificate;
	QThread* m_pThread;
	AppConfig& m_AppConfig;
};
#endif // PLUGINWIZARDPAGE_H

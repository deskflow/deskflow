/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2018 Symless Ltd.
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

#include "BonjourWindows.h"

#if defined(Q_OS_WIN)

#include "MainWindow.h"
#include "SettingsDialog.h"
#include "DataDownloader.h"
#include "QUtility.h"
#include "CommandProcess.h"

#include <QUrl>
#include <QThread>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BonjourWindows::BonjourWindows(
        SettingsDialog* settingsDialog,
        MainWindow* mainWindow,
        AppConfig& appConfig) :
    m_pSettingsDialog(settingsDialog),
    m_pMainWindow(mainWindow),
    m_pBonjourInstall(nullptr),
    m_pDownloadMessageBox(nullptr),
    m_pDataDownloader(nullptr),
    m_appConfig(appConfig)
{
}

BonjourWindows::~BonjourWindows()
{
    if (m_pBonjourInstall != nullptr) {
        delete m_pBonjourInstall;
    }

    if (m_pDownloadMessageBox != nullptr) {
        delete m_pDownloadMessageBox;
    }

    if (m_pDataDownloader != nullptr) {
        delete m_pDataDownloader;
    }
}

void BonjourWindows::downloadAndInstall()
{
    QUrl url;
    int arch = getProcessorArch();
    if (arch == kProcessorArchWin32) {
        url.setUrl(bonjourBaseUrl + bonjourFilename32);
        m_pMainWindow->appendLogInfo("downloading bonjour (32-bit)");
    }
    else if (arch == kProcessorArchWin64) {
        url.setUrl(bonjourBaseUrl + bonjourFilename64);
        m_pMainWindow->appendLogInfo("downloading bonjour (64-bit)");
    }
    else {
        QMessageBox::critical(
            m_pSettingsDialog, tr("Synergy Auto Config"),
            tr("Failed to detect system architecture."));
        return;
    }

    if (m_pDataDownloader == nullptr) {
        m_pDataDownloader = new DataDownloader(this);
        connect(m_pDataDownloader, SIGNAL(isComplete()), SLOT(downloadFinished()));
    }

    m_pDataDownloader->download(url);

    if (m_pDownloadMessageBox != nullptr) {
        delete m_pDownloadMessageBox;
        m_pDownloadMessageBox = nullptr;
    }

    m_pDownloadMessageBox = new QMessageBox(m_pSettingsDialog);
    m_pDownloadMessageBox->setWindowTitle("Synergy Auto Config");
    m_pDownloadMessageBox->setIcon(QMessageBox::Information);
    m_pDownloadMessageBox->setText("Installing Bonjour, please wait...");
    QAbstractButton* cancel = m_pDownloadMessageBox->addButton(
        tr("Cancel"), QMessageBox::RejectRole);

    m_pDownloadMessageBox->exec();

    if (cancel == m_pDownloadMessageBox->clickedButton()) {
        m_pDataDownloader->cancel();
    }
}

void BonjourWindows::downloadFinished()
{
    m_pMainWindow->appendLogInfo("bonjour downloaded");
    install();
}

void BonjourWindows::install()
{
    m_pMainWindow->appendLogInfo("installing bonjour");

    QString tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    QString filename = tempLocation;
    filename.append("\\").append(bonjourTargetFilename);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        m_pDownloadMessageBox->hide();

        QMessageBox::warning(
            m_pSettingsDialog, "Synergy",
            tr("Failed to download Bonjour installer to location: %1")
            .arg(tempLocation));
        return;
    }

    file.write(m_pDataDownloader->data());
    file.close();

    QStringList arguments;
    arguments.append("/i");
    QString winFilename = QDir::toNativeSeparators(filename);
    arguments.append(winFilename);
    arguments.append("/passive");

    if (m_pBonjourInstall != nullptr) {
        delete m_pBonjourInstall;
        m_pBonjourInstall = nullptr;
    }

    m_pBonjourInstall = new CommandProcess("msiexec", arguments);

    QThread* thread = new QThread;
    connect(m_pBonjourInstall, SIGNAL(finished()), this, SLOT(installFinished()));
    connect(m_pBonjourInstall, SIGNAL(finished()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    m_pBonjourInstall->moveToThread(thread);
    thread->start();

    QMetaObject::invokeMethod(m_pBonjourInstall, "run", Qt::QueuedConnection);

    m_pDownloadMessageBox->hide();
}

bool BonjourWindows::isRunning() const
{
    QString name = "Bonjour Service";

    SC_HANDLE hSCManager;
    hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);

    if (hSCManager == nullptr) {
        m_pMainWindow->appendLogError(
            QString("failed to open a service controller manager, error: %1").arg(GetLastError()));
        return false;
    }

    auto array = name.toLocal8Bit();
    SC_HANDLE hService = OpenService(hSCManager, array.data(), SERVICE_QUERY_STATUS);

    if (hService == nullptr) {
        m_pMainWindow->appendLogDebug(
            QString("failed to open service: %1").arg(name));
        return false;
    }

    SERVICE_STATUS status;
    if (QueryServiceStatus(hService, &status)) {
        if (status.dwCurrentState == SERVICE_RUNNING) {
            return true;
        }
    }

    return false;
}

void BonjourWindows::installFinished()
{
    m_pMainWindow->appendLogInfo("bonjour installed");
    m_appConfig.setAutoConfig(true);
    m_pSettingsDialog->allowAutoConfig();
}

#endif // Q_OS_WIN

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

#pragma once

#include <QtCore/QObject>

#if defined(Q_OS_WIN)

#include <QMessageBox>

static QString bonjourBaseUrl = "http://binaries.symless.com/bonjour/";
static const char bonjourFilename32[] = "Bonjour.msi";
static const char bonjourFilename64[] = "Bonjour64.msi";
static const char bonjourTargetFilename[] = "Bonjour.msi";

class SettingsDialog;
class MainWindow;
class CommandProcess;
class DataDownloader;
class AppConfig;

class BonjourWindows : public QObject
{
    Q_OBJECT

public:
    BonjourWindows(SettingsDialog* settingsDialog, MainWindow* mainWindow, AppConfig& appConfig);
    virtual ~BonjourWindows();

public:
    void downloadAndInstall();
    bool isRunning() const;

protected slots:
    void downloadFinished();
    void installFinished();

private:
    void install();

private:
    SettingsDialog* m_pSettingsDialog;
    MainWindow* m_pMainWindow;
    CommandProcess* m_pBonjourInstall;
    QMessageBox* m_pDownloadMessageBox;
    DataDownloader* m_pDataDownloader;
    AppConfig& m_appConfig;
};

#endif // Q_OS_WIN

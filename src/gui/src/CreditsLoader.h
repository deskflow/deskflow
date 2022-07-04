/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2022 Symless Ltd.
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

#include <QObject>
#include <QNetworkAccessManager>

class QNetworkReply;
class MainWindow;
class AppConfig;

class CreditsLoader : public QObject
{
    Q_OBJECT
public:
    explicit CreditsLoader(MainWindow& mainWindow, const AppConfig& config);
    void loadEliteBackers();

signals:
    void loaded(const QString& eliteBakers) const;

public slots:
    void replyFinished(QNetworkReply* reply) const;

private:
    MainWindow& m_mainWindow;
    const AppConfig& m_config;
    QNetworkAccessManager m_manager;

    void error(const QString& error) const;
};

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

#include "CreditsLoader.h"
#include "MainWindow.h"
#include "AppConfig.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonParseError>

namespace {

class CreditsResponse {
public:
    explicit CreditsResponse(const QByteArray& response) {
        auto data = parseJson(response);
        m_status = data["status"].toString();

        if (isSuccess()) {
            m_data = data["data"].toString();
        }
        else if (isFailed()) {
            m_data = data["message"].toString();
        }
        else {
            m_data = "Unknown response from the server";
        }
    }

    bool isSuccess() const {
        return (m_status == "success");
    }

    bool isFailed() const {
        return (m_status == "failed");
    }

    const QString& getData() const {
        return m_data;
    }

private:
    QMap<QString, QVariant> parseJson(const QByteArray& json) const {
        QJsonParseError jsonError;
        auto data =  QJsonDocument::fromJson(json, &jsonError);
        if (jsonError.error == QJsonParseError::NoError) {
            return data.toVariant().toMap();
        }
        return QMap<QString, QVariant>();
    }

    QString m_data;
    QString m_status;
};

}

CreditsLoader::CreditsLoader(MainWindow& mainWindow, const AppConfig& config) :
    m_mainWindow(mainWindow),
    m_config(config)
{
    connect(&m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
}

void CreditsLoader::loadEliteBackers()
{
    const auto ELITE_BACKERS_URL = m_config.getEliteBackersUrl();
    m_mainWindow.appendLogDebug(QString("Send request: %1").arg(ELITE_BACKERS_URL));
    const auto url = QUrl(ELITE_BACKERS_URL);
    const auto request = QNetworkRequest(url);
    m_manager.get(request);
}

void CreditsLoader::replyFinished(QNetworkReply* reply) const
{
    auto response = reply->readAll();
    m_mainWindow.appendLogDebug(QString("Received response: %1").arg(response.toStdString().c_str()));

    CreditsResponse creditsResponse(response);
    if (creditsResponse.isSuccess()) {
        emit loaded(creditsResponse.getData());
    }
    else {
        error(QString("Server error: %1").arg(creditsResponse.getData()));
    }
}

void CreditsLoader::error(const QString& error) const
{
    m_mainWindow.appendLogError(error);
    emit loaded("Something went wrong when retrieving the list of Elite Backers. Check the logs for more info.");
}

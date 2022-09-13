/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Synergy Seamless Inc.
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
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>

#include "LicenseRegistry.h"
#include "AppConfig.h"
#include <QSysInfo>

LicenseRegistry::LicenseRegistry(AppConfig& config) :
    m_config(config)
{
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(registerLicense()));
}

void LicenseRegistry::registerLicense()
{
    m_timer.stop();
    if (m_config.edition() == Edition::kBusiness) {
        const auto REGISTER_LICENSE_URL = m_config.getLicenseRegistryUrl();
        const auto url = QUrl(REGISTER_LICENSE_URL);

        auto request = QNetworkRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        m_manager.post(request, getRequestData());
        connect(&m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleResponse(QNetworkReply*)));
    }
}

void LicenseRegistry::handleResponse(QNetworkReply *reply)
{
    if (reply) {
        const auto HOUR = (60*60); //seconds per hour
        const auto DAY = (24*HOUR);//seconds per day
        const auto WEEK = (7*DAY); //seconds per week
        const auto currentTimestamp = time(nullptr);

        if(reply->error() == QNetworkReply::NoError) {
            m_config.setLicenseNextCheck(currentTimestamp + WEEK);
        }
        else{
            m_config.setLicenseNextCheck(currentTimestamp + HOUR);
        }

        scheduleRegistration();
        reply->deleteLater();
    }
}

QByteArray LicenseRegistry::getRequestData() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    QString guid(QSysInfo::machineUniqueId());
#else
    QString guid;
#endif

    QJsonObject data;
    if (!guid.isEmpty()) {
        data["guid"] = guid;
        data["guid_type"] = "system";
    }
    else {
        data["guid"] = m_config.getGuid();
        data["guid_type"] = "synergy";
    }

    data["key"] = m_config.serialKey();
    data["is_server"] = m_config.getServerGroupChecked();

    return  QJsonDocument(data).toJson();
}

void LicenseRegistry::scheduleRegistration()
{
    const auto nextCheck = m_config.getLicenseNextCheck();
    const auto currentTimestamp = static_cast<unsigned long long>(time(nullptr));

    if (currentTimestamp >= nextCheck) {
        registerLicense();
    }
    else {
        const auto interval = (nextCheck - currentTimestamp) * 1000; //interval in milliseconds
        m_timer.setInterval(static_cast<int>(interval));
        m_timer.setSingleShot(true);
        m_timer.start();
    }
}

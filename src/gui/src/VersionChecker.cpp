/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "VersionChecker.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QLocale>

#define VERSION_URL "https://api.symless.com/version"

VersionChecker::VersionChecker()
{
    m_manager = new QNetworkAccessManager(this);

    connect(m_manager, SIGNAL(finished(QNetworkReply*)),
         this, SLOT(replyFinished(QNetworkReply*)));
}

VersionChecker::~VersionChecker()
{
    delete m_manager;
}

void VersionChecker::checkLatest()
{
    auto request = QNetworkRequest(QUrl(VERSION_URL));
    request.setHeader(QNetworkRequest::UserAgentHeader, QString("Synergy (") + SYNERGY_VERSION + ") " + QSysInfo::prettyProductName());
    request.setRawHeader("X-Synergy-Version", SYNERGY_VERSION );
    request.setRawHeader("X-Synergy-Language", QLocale::system().name().toStdString().c_str() );
    m_manager->get(request);
}

void VersionChecker::replyFinished(QNetworkReply* reply)
{
    auto newestVersion = QString(reply->readAll());
    if (!newestVersion.isEmpty() && compareVersions(SYNERGY_VERSION, newestVersion) > 0) {
        emit updateFound(newestVersion);
    }
}

int VersionChecker::getStageVersion(QString stage) const
{
    const char* stableName = "stable";
    const char* rcName = "rc";
    const char* betaName = "beta";

    // use max int for stable so it's always the highest value.
    const int stableValue = INT_MAX;
    const int rcValue = 2;
    const int betaValue = 1;
    const int otherValue = 0;

    if (stage == stableName)
    {
        return stableValue;
    }
    else if (stage.toLower().startsWith(rcName))
    {
        QRegExp rx("\\d*", Qt::CaseInsensitive);
        if (rx.indexIn(stage) != -1)
        {
            // return the rc value plus the rc number (e.g. 2 + 1)
            // this should be ok since stable is max int.
            return rcValue + rx.cap(1).toInt();
        }
    }
    else if (stage == betaName)
    {
        return betaValue;
    }
    
    return otherValue;
}

int VersionChecker::compareVersions(const QString& left, const QString& right)
{
    if (left.compare(right) == 0)
        return 0; // versions are same.

    QStringList leftParts = left.split("-");
    QStringList rightParts = right.split("-");

    if (leftParts.size() < 1 || rightParts.size() < 1)
        return 0; // versions are same.

    QString leftNumber = leftParts.at(0);
    QString rightNumber = rightParts.at(0);

    QStringList leftNumberParts = left.split(".");
    QStringList rightNumberParts = right.split(".");

    const int leftMajor = leftNumberParts.at(0).toInt();
    const int leftMinor = leftNumberParts.at(1).toInt();
    const int leftPatch = leftNumberParts.at(2).toInt();
    const int leftStage = leftParts.size() > 1 ? getStageVersion(leftParts.at(1)) : 0;

    const int rightMajor = rightNumberParts.at(0).toInt();
    const int rightMinor = rightNumberParts.at(1).toInt();
    const int rightPatch = rightNumberParts.at(2).toInt();
    const int rightStage = rightParts.size() > 1 ? getStageVersion(rightParts.at(1)) : 0;

    const bool rightWins =
        ( rightMajor >  leftMajor) ||
        ((rightMajor >= leftMajor) && (rightMinor > leftMinor)) ||
        ((rightMajor >= leftMajor) && (rightMinor >= leftMinor) && (rightPatch > leftPatch)) ||
        ((rightMajor >= leftMajor) && (rightMinor >= leftMinor) && (rightPatch >= leftPatch) && (rightStage > leftStage));

    return rightWins ? 1 : -1;
}


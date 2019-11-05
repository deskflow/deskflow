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

#define VERSION_REGEX "(\\d+\\.\\d+\\.\\d+-[a-z1-9]*)"
#define VERSION_REGEX_SECTIONED "(\\d+)\\.(\\d+)\\.(\\d+)-([a-z1-9]*)"
#define VERSION_SEGMENT_COUNT 4
#define VERSION_URL "https://version.symless.com/synergy"


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
    request.setHeader(QNetworkRequest::UserAgentHeader, QString("Synergy (") + getVersion() + ") " + QSysInfo::prettyProductName());
    request.setRawHeader("X-Synergy-Version", getVersion().toStdString().c_str() );
    m_manager->get(request);
}

void VersionChecker::replyFinished(QNetworkReply* reply)
{
    QString newestVersion = QString(reply->readAll());
    if (!newestVersion.isEmpty())
    {
        QString currentVersion = getVersion();
        if (currentVersion != "Unknown") {
            if (compareVersions(currentVersion, newestVersion) > 0)
                emit updateFound(newestVersion);
        }
    }
}

int VersionChecker::getStageVersion(QString stage)
{
    const int valueStable   = INT_MAX; //Stable will always be considered the highest value
    const int valueRC       = 2;
    const int valueSnapshot = 1;
    const int valueOther    = 0;

    //Stable should always be considered highest, followed by rc[0-9] then snapshots with everything else at the end
    //HACK There is probably a much better way of doing this
    if (stage == "stable")
    {
        return valueStable;
    }
    else if (stage.startsWith("rc") || stage.startsWith("RC"))
    {
        QRegExp rx("\\d*", Qt::CaseInsensitive);
        if (rx.indexIn(stage) != -1)
        {
            //Return the RC value plus the RC version as in int 
            return valueRC + rx.cap(1).toInt();
        }
    }
    else if (stage == "snapshot")
    {
        return valueSnapshot;
    }

    return valueOther;
}

int VersionChecker::compareVersions(const QString& left, const QString& right)
{
    if (left.compare(right) == 0)
        return 0; // versions are same.

    QStringList leftSplit = left.split(QRegExp("[\\.-]"));
    if (leftSplit.size() != VERSION_SEGMENT_COUNT)
        return 1; // assume right wins.

    QStringList rightSplit = right.split(QRegExp("[\\.-]"));
    if (rightSplit.size() != VERSION_SEGMENT_COUNT)
        return -1; // assume left wins.

    const int leftMajor = leftSplit.at(0).toInt();
    const int leftMinor = leftSplit.at(1).toInt();
    const int leftRev   = leftSplit.at(2).toInt();
    const int leftStage = getStageVersion(leftSplit.at(3));

    const int rightMajor = rightSplit.at(0).toInt();
    const int rightMinor = rightSplit.at(1).toInt();
    const int rightRev   = rightSplit.at(2).toInt();
    const int rightStage = getStageVersion(rightSplit.at(3));

    const bool rightWins =
        ( rightMajor >  leftMajor) ||
        ((rightMajor >= leftMajor) && (rightMinor > leftMinor)) ||
        ((rightMajor >= leftMajor) && (rightMinor >= leftMinor) && (rightRev > leftRev)) ||
        ((rightMajor >= leftMajor) && (rightMinor >= leftMinor) && (rightRev >= leftRev) && (rightStage > leftStage));

    return rightWins ? 1 : -1;
}

QString VersionChecker::getVersion()
{
    QProcess process;
    process.start(m_app, QStringList() << "--version");

    process.setReadChannel(QProcess::StandardOutput);
    if (process.waitForStarted() && process.waitForFinished())
    {
        QRegExp rx(VERSION_REGEX,Qt::CaseInsensitive);
        QString text = process.readLine();
        if (rx.indexIn(text) != -1)
        {
            return rx.cap(1);
        }
    }

    return tr("Unknown");
}

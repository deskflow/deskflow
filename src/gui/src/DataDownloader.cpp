/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, Inc.
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

#include "DataDownloader.h"

DataDownloader::DataDownloader(QUrl url, QObject* parent) :
	QObject(parent)
{
	connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)),
				SLOT(fileDownloaded(QNetworkReply*)));

	QNetworkRequest request(url);
	m_WebCtrl.get(request);
}

DataDownloader::~DataDownloader()
{

}

void DataDownloader::fileDownloaded(QNetworkReply* reply)
{
	m_DownloadedData = reply->readAll();
	reply->deleteLater();
	emit downloaded();
}

QByteArray DataDownloader::downloadedData() const
{
	return m_DownloadedData;
}

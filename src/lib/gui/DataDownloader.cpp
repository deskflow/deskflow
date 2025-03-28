/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DataDownloader.h"

DataDownloader::DataDownloader(QObject *parent) : QObject(parent), m_pReply(nullptr), m_IsFinished(false)
{
  connect(&m_NetworkManager, &QNetworkAccessManager::finished, this, &DataDownloader::complete);
}

DataDownloader::~DataDownloader()
{
}

void DataDownloader::complete(QNetworkReply *reply)
{
  m_Data = reply->readAll();
  reply->deleteLater();
  m_pReply = nullptr;

  if (!m_Data.isEmpty()) {
    m_IsFinished = true;
    Q_EMIT isComplete();
  }
}

QByteArray DataDownloader::data() const
{
  return m_Data;
}

void DataDownloader::cancel()
{
  if (m_pReply != nullptr) {
    m_pReply->abort();
  }
}

void DataDownloader::download(QUrl url)
{
  QNetworkRequest request(url);
  m_pReply = m_NetworkManager.get(request);
}

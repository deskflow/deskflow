/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTcpSocket>

/**
 * @brief Useful for overriding QDataStream.
 */
class QDataStreamProxy
{
public:
  explicit QDataStreamProxy() = default;
  explicit QDataStreamProxy(QTcpSocket *socket) : m_Stream(std::make_unique<QDataStream>(socket))
  {
  }
  virtual ~QDataStreamProxy() = default;

  virtual qint64 writeRawData(const char *data, int len)
  {
    assert(m_Stream);
    return m_Stream->writeRawData(data, len);
  }

private:
  std::unique_ptr<QDataStream> m_Stream;
};

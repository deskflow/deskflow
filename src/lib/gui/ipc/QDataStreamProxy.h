/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

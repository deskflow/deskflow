/*
 * synergy -- mouse and keyboard sharing utility
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

#include <QDataStream>
#include <QTcpSocket>
#include <qglobal.h>
// #include <iostream>
// #include <memory>

class StreamProvider {
public:
  class StreamProxy {
  public:
    explicit StreamProxy(QTcpSocket *socket = nullptr)
        : m_Stream(new QDataStream(socket)) {}

    virtual ~StreamProxy() = default;

    virtual int writeRawData(const char *data, int size) {
      assert(m_Stream);
      return m_Stream->writeRawData(data, size);
    }

  private:
    QDataStream *m_Stream;
  };

public:
  explicit StreamProvider(QTcpSocket *socket = nullptr) : m_Socket(socket) {}
  virtual ~StreamProvider() = default;

  virtual StreamProxy *makeStream() {
    return new StreamProxy(m_Socket); //
  }

private:
  QTcpSocket *m_Socket;
};

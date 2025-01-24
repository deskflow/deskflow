/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QtGlobal>

namespace deskflow::gui {

inline int bytesToInt(const char *buffer, size_t size)
{
  QByteArray byteArray(buffer, static_cast<int>(size));
  QDataStream stream(byteArray);
  int result;
  stream >> result;
  return result;
}

inline QByteArray intToBytes(int value)
{
  QByteArray bytes;
  QDataStream stream(&bytes, QIODevice::WriteOnly);
  stream << value;
  return bytes;
}

} // namespace deskflow::gui

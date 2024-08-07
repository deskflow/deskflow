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

#include <qlogging.h>

namespace synergy::gui {

// TODO: use QDataStream instead of raw bytes
inline int bytesToInt(const char *buffer, int size) {
  if (size == 1) {
    return (unsigned char)buffer[0];
  } else if (size == 2) {
    return (((unsigned char)buffer[0]) << 8) + (unsigned char)buffer[1];
  } else if (size == 4) {
    return (((unsigned char)buffer[0]) << 24) +
           (((unsigned char)buffer[1]) << 16) +
           (((unsigned char)buffer[2]) << 8) + (unsigned char)buffer[3];
  } else {
    qFatal("bytesToInt: size must be 1, 2, or 4");
    return 0;
  }
}

// TODO: use QDataStream instead of raw bytes
inline void intToBytes(int value, char *buffer, int size) {
  if (size == 1) {
    buffer[0] = value & 0xff;
  } else if (size == 2) {
    buffer[0] = (value >> 8) & 0xff;
    buffer[1] = value & 0xff;
  } else if (size == 4) {
    buffer[0] = (value >> 24) & 0xff;
    buffer[1] = (value >> 16) & 0xff;
    buffer[2] = (value >> 8) & 0xff;
    buffer[3] = value & 0xff;
  } else {
    qFatal("intToBytes: size must be 1, 2, or 4");
  }
}

} // namespace synergy::gui
